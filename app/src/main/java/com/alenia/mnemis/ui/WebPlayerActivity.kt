package com.alenia.mnemis.ui

import android.annotation.SuppressLint
import android.app.PictureInPictureParams
import android.content.Intent
import android.content.res.Configuration
import android.graphics.Bitmap
import android.os.Build
import android.os.Bundle
import android.util.Rational
import android.view.View
import android.webkit.WebChromeClient
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.ActivityWebPlayerBinding
import com.alenia.mnemis.databinding.DialogKoalasyncBinding
import com.alenia.mnemis.plugin.DynamicPluginManager
import com.alenia.mnemis.plugin.MnemisPluginRegistry
import com.alenia.mnemis.playback.PlaybackService
import com.alenia.mnemis.prefs.MnemisPreferences
import com.alenia.mnemis.resolver.MnemisMediaResolver
import com.alenia.mnemis.sync.KoalaSyncClient
import com.alenia.mnemis.sync.KoalaSyncWebBridge
import com.alenia.mnemis.sync.WebAdBlocker
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * WebView-based player with injected KoalaSync plugin script for web streaming platforms.
 *
 * Fullscreen uses the native WebView fullscreen custom view
 * (WebChromeClient.onShowCustomView). That is the path YouTube supports and
 * the one that worked for watching in v1.0.3 and earlier. v1.0.6 replaced it
 * with in-app fullscreen (a JS Fullscreen API override that forced a CSS
 * class onto the player), but the container went fullscreen black while the
 * inner video did not scale to fill it, so fullscreen watching showed a
 * huge black box instead of the video. That override is reverted here:
 * watching in fullscreen must never break to fix PiP.
 *
 * PiP: for non-YouTube platforms the fullscreen custom view is LEFT
 * SHOWING when entering PiP (v1.0.4 hid it first and v1.0.5 hid it after
 * entering PiP; both destroyed the video surface and left the PiP window
 * black). YouTube is different: PiP does NOT work on YouTube in a WebView
 * at all -- all four approaches failed on YouTube specifically (Josue:
 * "Todo literal el pip no sirve en yotube"), while other platforms work.
 * YouTube's #movie_player, its own fullscreen handling and its
 * visibilitychange pause behavior make the WebView video surface
 * unusable for PiP.
 *
 * So for YouTube videos, PiP hands playback over to the native Media3
 * PlayerActivity with a direct stream URL resolved via
 * MnemisMediaResolver (InnerTube API), which has proper PiP support.
 * The stream URL is pre-resolved in the background while the video plays
 * (auto-PiP on Home cannot wait for a network resolve: by the time it
 * finished, the app would already be in the background and the activity
 * launch would be blocked), and the playback position is polled every 2s
 * so the native player resumes where the WebView video was. If no stream
 * URL can be resolved (YouTube changed its API, age-restricted video,
 * ...), PiP falls back to the WebView path. If the user is in a KoalaSync
 * room, the room credentials are passed to the native player so it
 * rejoins the same room.
 */
class WebPlayerActivity : AppCompatActivity() {

    companion object {
        const val EXTRA_WEB_URL = "extra_web_url"
        const val EXTRA_WEB_TITLE = "extra_web_title"
    }

    private lateinit var binding: ActivityWebPlayerBinding
    private lateinit var koalaSyncClient: KoalaSyncClient
    private lateinit var webBridge: KoalaSyncWebBridge
    private lateinit var prefs: MnemisPreferences
    private var customView: View? = null
    private var customViewCallback: WebChromeClient.CustomViewCallback? = null
    private var currentPageUrl: String = ""
    private var isInPipMode: Boolean = false

    // YouTube PiP handover state (see class KDoc).
    private var cachedYouTubeVideoId: String? = null
    private var cachedYouTubeStreamUrl: String? = null
    private var resolvingYouTubeVideoId: String? = null
    private var latestPositionMs: Long = 0L
    private var snapshotPollJob: Job? = null
    private var pipHandoverInProgress: Boolean = false

    // Room credentials, remembered from the join dialog so a YouTube PiP
    // handover can rejoin the same room from the native player.
    private var activeRoomId: String? = null
    private var activeRoomUsername: String? = null
    private var activeRoomPassword: String? = null
    private var activeRoomChatSecret: String? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityWebPlayerBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val targetUrl = intent.getStringExtra(EXTRA_WEB_URL)
        if (targetUrl.isNullOrBlank()) {
            Toast.makeText(this, "URL inválida", Toast.LENGTH_SHORT).show()
            finish()
            return
        }

        currentPageUrl = targetUrl
        val initialTitle = intent.getStringExtra(EXTRA_WEB_TITLE) ?: targetUrl
        binding.tvWebTitle.text = initialTitle

        setupSyncAndBridge()
        DynamicPluginManager.loadAll(this)
        prefs.saveLastPlayed(initialTitle, targetUrl, isWeb = true)
        setupWebView(targetUrl)
        setupControls()
    }

    override fun onResume() {
        super.onResume()
        startSnapshotPolling()
    }

    override fun onPause() {
        stopSnapshotPolling()
        super.onPause()
    }

    private fun setupSyncAndBridge() {
        prefs = MnemisPreferences(this)
        binding.btnWebKoalaSync.visibility = if (prefs.koalaSyncPluginEnabled) View.VISIBLE else View.GONE
        koalaSyncClient = KoalaSyncClient(
            serverUrl = prefs.relayServerUrl,
            peerId = prefs.getOrCreatePeerId(),
            listener = object : KoalaSyncClient.SyncListener {
                override fun onRemotePlay(senderId: String, currentTimeSeconds: Double?) {
                    runOnUiThread {
                        webBridge.applyRemotePlay(currentTimeSeconds)
                    }
                }

                override fun onRemotePause(senderId: String, currentTimeSeconds: Double?) {
                    runOnUiThread {
                        webBridge.applyRemotePause(currentTimeSeconds)
                    }
                }

                override fun onRemoteSeek(senderId: String, targetTimeSeconds: Double) {
                    runOnUiThread {
                        webBridge.applyRemoteSeek(targetTimeSeconds)
                    }
                }

                override fun onRemoteDriftCheck(senderId: String, expectedTimeSeconds: Double) {
                    runOnUiThread {
                        webBridge.correctDriftIfNeeded(expectedTimeSeconds)
                    }
                }

                override fun onSyncError(message: String) {
                    runOnUiThread {
                        Toast.makeText(this@WebPlayerActivity, "KoalaSync: $message", Toast.LENGTH_LONG).show()
                    }
                }
            }
        )

        webBridge = KoalaSyncWebBridge(
            webView = binding.webViewPlayer,
            syncClient = koalaSyncClient,
            onRunOnUiThread = { action -> runOnUiThread(action) }
        )

        koalaSyncClient.setSnapshotProvider {
            webBridge.getLatestSnapshot()
        }

        lifecycleScope.launch {
            koalaSyncClient.roomState.collect { state ->
                val tintColorRes = when (state.connectionState) {
                    KoalaSyncClient.ConnectionState.CONNECTED -> R.color.teal_200
                    KoalaSyncClient.ConnectionState.CONNECTING -> R.color.accent_indicator
                    KoalaSyncClient.ConnectionState.DISCONNECTED -> R.color.white
                }
                binding.btnWebKoalaSync.setColorFilter(
                    ContextCompat.getColor(this@WebPlayerActivity, tintColorRes)
                )
                binding.btnWebChat.visibility = if (state.connectionState == KoalaSyncClient.ConnectionState.CONNECTED) {
                    View.VISIBLE
                } else {
                    View.GONE
                }
                binding.layoutConnectingOverlay.visibility = if (state.connectionState == KoalaSyncClient.ConnectionState.CONNECTING) {
                    View.VISIBLE
                } else {
                    View.GONE
                }
                if (state.connectionState == KoalaSyncClient.ConnectionState.CONNECTED) {
                    PlaybackService.start(this@WebPlayerActivity, binding.tvWebTitle.text?.toString())
                } else if (state.connectionState == KoalaSyncClient.ConnectionState.DISCONNECTED) {
                    PlaybackService.stop(this@WebPlayerActivity)
                }
            }
        }
    }

    private fun injectMatchingWebPlugins(pageUrl: String) {
        if (pageUrl.isBlank()) return
        val plugins = MnemisPluginRegistry.findMatchingWebPlugins(pageUrl, prefs)
        for (plugin in plugins) {
            val script = plugin.getUserScript(pageUrl)
            if (!script.isNullOrBlank()) {
                binding.webViewPlayer.evaluateJavascript(script, null)
            }
        }
    }

    /** YouTube video ID of the current page, or null when the page is not a YouTube video. */
    private fun currentYouTubeVideoId(): String? =
        MnemisMediaResolver.extractYouTubeVideoId(currentPageUrl)

    /**
     * Pre-resolves the direct stream URL for the current YouTube video in
     * the background and caches it, so a PiP request (button or Home) can
     * hand over to the native player instantly. Auto-PiP on Home cannot
     * wait for a network resolve: by the time it finished, the app would
     * already be in the background and the activity launch would be
     * blocked. Resolving early while the video plays avoids that.
     * Only one resolve runs per video ID; a failed resolve simply leaves
     * the cache empty and PiP falls back to the WebView behavior.
     */
    private fun maybePreResolveYouTubeStream() {
        val videoId = currentYouTubeVideoId() ?: return
        if (videoId == cachedYouTubeVideoId) return
        if (videoId == resolvingYouTubeVideoId) return
        // New video: drop the old stream URL and position.
        cachedYouTubeVideoId = null
        cachedYouTubeStreamUrl = null
        latestPositionMs = 0L
        resolvingYouTubeVideoId = videoId
        val pageUrl = currentPageUrl
        lifecycleScope.launch(Dispatchers.IO) {
            val resolved = try {
                MnemisMediaResolver.resolve(pageUrl)
            } catch (_: Exception) {
                null
            }
            withContext(Dispatchers.Main) {
                if (resolvingYouTubeVideoId != videoId) return@withContext
                resolvingYouTubeVideoId = null
                if (resolved != null && resolved != pageUrl.trim() && currentYouTubeVideoId() == videoId) {
                    cachedYouTubeVideoId = videoId
                    cachedYouTubeStreamUrl = resolved
                }
            }
        }
    }

    /**
     * Keeps the current playback position fresh (at most ~2s stale) while
     * a YouTube video plays, so the native player can resume at the right
     * place on handover. The bridge only reports position on play/pause/
     * seek events, which can be minutes stale mid-playback; a live JS
     * query at handover time is too slow for auto-PiP on Home.
     */
    private fun startSnapshotPolling() {
        snapshotPollJob?.cancel()
        snapshotPollJob = lifecycleScope.launch {
            while (isActive) {
                delay(2000)
                try {
                    if (currentYouTubeVideoId() != null && webBridge.isVideoPlaying()) {
                        val snap = webBridge.getLatestSnapshot()
                        latestPositionMs = (snap.currentTimeSeconds * 1000.0).toLong().coerceAtLeast(0L)
                    }
                } catch (_: Exception) {
                    // Polling is best-effort; a failed query just keeps the last position.
                }
            }
        }
    }

    private fun stopSnapshotPolling() {
        snapshotPollJob?.cancel()
        snapshotPollJob = null
    }

    /**
     * Hands a YouTube video over to the native Media3 player and finishes
     * this activity. The native player starts at the last known position
     * and enters PiP as soon as it is playing, so the user ends up in a
     * PiP window with the video rendering from ExoPlayer instead of the
     * WebView surface that YouTube breaks. If the user is in a KoalaSync
     * room, the room credentials are passed along so the native player
     * rejoins the same room (the old client disconnects when this activity
     * finishes; peers briefly see one peer leave and another join).
     */
    private fun launchNativePlayerForYouTube(streamUrl: String) {
        val title = binding.tvWebTitle.text?.toString() ?: currentPageUrl
        val intent = Intent(this, PlayerActivity::class.java).apply {
            addFlags(Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TOP)
            putExtra(PlayerActivity.EXTRA_VIDEO_URI, streamUrl)
            putExtra(PlayerActivity.EXTRA_VIDEO_TITLE, title)
            putExtra(PlayerActivity.EXTRA_START_POSITION_MS, latestPositionMs)
            putExtra(PlayerActivity.EXTRA_ENTER_PIP_ON_READY, true)
            val roomState = koalaSyncClient.roomState.value
            if (roomState.connectionState != KoalaSyncClient.ConnectionState.DISCONNECTED && activeRoomId != null) {
                putExtra(PlayerActivity.EXTRA_ROOM_ID, activeRoomId)
                putExtra(PlayerActivity.EXTRA_ROOM_USERNAME, activeRoomUsername)
                putExtra(PlayerActivity.EXTRA_ROOM_PASSWORD, activeRoomPassword)
                putExtra(
                    PlayerActivity.EXTRA_ROOM_CHAT_SECRET,
                    koalaSyncClient.getActiveChatSecret() ?: activeRoomChatSecret
                )
            }
        }
        startActivity(intent)
        finish()
    }

    /**
     * Handles a PiP request, from the PiP button ([fromUserTap] = true) or
     * from auto-PiP on Home ([fromUserTap] = false).
     *
     * YouTube is special: PiP does NOT work on YouTube in a WebView at
     * all (v1.0.4 hide-first, v1.0.5 hide-after, v1.0.6 in-app fullscreen
     * and v1.0.7 leave-custom-view-showing all failed on YouTube, while
     * other platforms work). For YouTube videos, PiP hands playback over
     * to the native Media3 player instead, which has proper PiP support.
     * The hand over needs a resolved direct stream URL: it is pre-resolved
     * in the background while the video plays, so the common case is
     * instant. The button tap runs in the foreground, so it can resolve
     * on demand and wait; auto-PiP on Home cannot wait (the app is
     * already backgrounding), so it only uses the synchronous fast path
     * and otherwise falls back to WebView PiP.
     */
    private fun onPipRequested(fromUserTap: Boolean) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            if (fromUserTap) {
                Toast.makeText(this, "PiP no compatible con esta versión de Android", Toast.LENGTH_SHORT).show()
            }
            return
        }
        if (isInPipMode) return

        val videoId = currentYouTubeVideoId()
        if (videoId != null) {
            val cached = if (cachedYouTubeVideoId == videoId) cachedYouTubeStreamUrl else null
            if (cached != null) {
                launchNativePlayerForYouTube(cached)
                return
            }
            if (fromUserTap && !pipHandoverInProgress) {
                pipHandoverInProgress = true
                Toast.makeText(this, "Preparando PiP…", Toast.LENGTH_SHORT).show()
                val pageUrl = currentPageUrl
                lifecycleScope.launch(Dispatchers.IO) {
                    val resolved = try {
                        MnemisMediaResolver.resolve(pageUrl)
                    } catch (_: Exception) {
                        null
                    }
                    withContext(Dispatchers.Main) {
                        pipHandoverInProgress = false
                        if (isFinishing || isDestroyed) return@withContext
                        if (resolved != null && resolved != pageUrl.trim() && currentYouTubeVideoId() == videoId) {
                            cachedYouTubeVideoId = videoId
                            cachedYouTubeStreamUrl = resolved
                            launchNativePlayerForYouTube(resolved)
                        } else {
                            Toast.makeText(
                                this@WebPlayerActivity,
                                "No se pudo preparar el video para PiP",
                                Toast.LENGTH_SHORT
                            ).show()
                            enterWebViewPip()
                        }
                    }
                }
                return
            }
            // Auto-PiP with nothing cached yet (video just started): the
            // pre-resolve normally fills the cache while the video plays.
            // Fall back to WebView PiP; it is known not to work on
            // YouTube, but it is better than doing nothing.
            enterWebViewPip()
            return
        }

        // Not a YouTube video: WebView PiP as before.
        enterWebViewPip()
    }

    /**
     * The plain WebView PiP path (fullscreen custom view left showing).
     * Works for non-YouTube platforms; kept as the fallback for YouTube
     * when no direct stream URL could be resolved.
     */
    private fun enterWebViewPip() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O || isInPipMode) return
        try {
            val builder = PictureInPictureParams.Builder()
                .setAspectRatio(Rational(16, 9))
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
                builder.setAutoEnterEnabled(true)
                builder.setSeamlessResizeEnabled(true)
            }
            enterPictureInPictureMode(builder.build())
        } catch (_: Exception) {
            // Ignorar fallo de PiP en dispositivos que no lo soporten en este estado
        }
    }

    @SuppressLint("SetJavaScriptEnabled")
    private fun setupWebView(url: String) {
        binding.webViewPlayer.apply {
            settings.javaScriptEnabled = true
            settings.domStorageEnabled = true
            settings.mediaPlaybackRequiresUserGesture = false
            settings.mixedContentMode = WebSettings.MIXED_CONTENT_COMPATIBILITY_MODE
            settings.userAgentString =
                "Mozilla/5.0 (Linux; Android 13; Mobile) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Mobile Safari/537.36"

            addJavascriptInterface(webBridge, KoalaSyncWebBridge.JS_INTERFACE_NAME)

            webViewClient = object : WebViewClient() {
                override fun shouldInterceptRequest(
                    view: WebView?,
                    request: WebResourceRequest?
                ): WebResourceResponse? {
                    val reqUrl = request?.url?.toString()
                    if (reqUrl != null) {
                        val blockedByPlugin = MnemisPluginRegistry
                            .findMatchingWebPlugins(currentPageUrl, prefs)
                            .any { it.shouldBlockRequest(reqUrl) }
                        if (blockedByPlugin || (prefs.adBlockEnabled && WebAdBlocker.shouldBlock(reqUrl))) {
                            return WebAdBlocker.emptyResponse()
                        }
                    }
                    return super.shouldInterceptRequest(view, request)
                }

                override fun onPageStarted(view: WebView?, url: String?, favicon: Bitmap?) {
                    super.onPageStarted(view, url, favicon)
                    if (!url.isNullOrBlank()) {
                        currentPageUrl = url
                    }
                    maybePreResolveYouTubeStream()
                    if (prefs.adBlockEnabled) {
                        webBridge.injectEarlyAdBlock()
                    }
                }

                override fun onPageFinished(view: WebView?, loadedUrl: String?) {
                    super.onPageFinished(view, loadedUrl)
                    val activeUrl = loadedUrl ?: currentPageUrl
                    currentPageUrl = activeUrl
                    maybePreResolveYouTubeStream()
                    if (prefs.adBlockEnabled) {
                        webBridge.injectEarlyAdBlock()
                    }
                    injectMatchingWebPlugins(activeUrl)
                    if (prefs.koalaSyncPluginEnabled) {
                        webBridge.injectScript()
                    }
                    updateSnifferVisibility()
                    val title = view?.title
                    if (!title.isNullOrBlank()) {
                        binding.tvWebTitle.text = title
                        prefs.saveLastPlayed(title, activeUrl, isWeb = true)
                    }
                }

                override fun doUpdateVisitedHistory(view: WebView?, url: String?, isReload: Boolean) {
                    super.doUpdateVisitedHistory(view, url, isReload)
                    val activeUrl = url ?: currentPageUrl
                    currentPageUrl = activeUrl
                    maybePreResolveYouTubeStream()
                    injectMatchingWebPlugins(activeUrl)
                    updateSnifferVisibility()
                    val isYouTube = activeUrl.contains("youtube.com", ignoreCase = true) ||
                        activeUrl.contains("youtu.be", ignoreCase = true)
                    val allowSpaBridge = !isYouTube || prefs.isPluginEnabled(MnemisPreferences.PLUGIN_YOUTUBE_SPA)
                    if (prefs.koalaSyncPluginEnabled && allowSpaBridge) {
                        webBridge.injectScript()
                    }
                }
            }

            webChromeClient = object : WebChromeClient() {
                override fun onShowCustomView(view: View?, callback: CustomViewCallback?) {
                    // Native WebView fullscreen: the video moves into the
                    // fullscreen custom view. This is the path YouTube
                    // supports and the one that worked for watching before
                    // v1.0.6. The v1.0.6 JS Fullscreen API override that
                    // replaced it is reverted: it left a huge black box in
                    // fullscreen because the inner video did not scale to
                    // fill the forced container.
                    if (customView != null) {
                        callback?.onCustomViewHidden()
                        return
                    }
                    customView = view
                    customViewCallback = callback
                    binding.fullscreenContainer.addView(view)
                    binding.fullscreenContainer.visibility = View.VISIBLE
                    binding.webViewPlayer.visibility = View.GONE
                    binding.webTopBar.visibility = View.GONE
                }

                override fun onHideCustomView() {
                    binding.fullscreenContainer.removeAllViews()
                    binding.fullscreenContainer.visibility = View.GONE
                    binding.webViewPlayer.visibility = View.VISIBLE
                    // Do not restore the top bar while in PiP: it would cover the
                    // video in the small PiP window. onPictureInPictureModeChanged
                    // owns the top bar visibility while in PiP.
                    if (!isInPipMode) {
                        binding.webTopBar.visibility = View.VISIBLE
                    }
                    customView = null
                    customViewCallback?.onCustomViewHidden()
                    customViewCallback = null
                    // Leaving fullscreen moves the video back to the inline
                    // player and can pause it; make sure it keeps playing.
                    resumeVideo()
                }
            }

            loadUrl(url)
        }
    }

    private fun setupControls() {
        binding.btnWebBack.setOnClickListener {
            if (binding.webViewPlayer.canGoBack()) {
                binding.webViewPlayer.goBack()
            } else {
                finish()
            }
        }

        binding.btnWebChat.setOnClickListener {
            KoalaSyncChatDialogHelper.show(this, lifecycleScope, koalaSyncClient)
        }

        binding.btnWebKoalaSync.setOnClickListener {
            showKoalaSyncDialog()
        }

        binding.btnWebSniffer.setOnClickListener {
            val streams = com.alenia.mnemis.plugin.HlsSnifferPlugin.getDetectedStreams()
            if (streams.isEmpty()) {
                Toast.makeText(this, "No se detectaron streams reproducibles en esta página", Toast.LENGTH_SHORT).show()
                return@setOnClickListener
            }
            val streamLabels = streams.map { "[${it.kind}] ${it.url.take(55)}…" }.toTypedArray()
            AlertDialog.Builder(this)
                .setTitle("Streams directos detectados")
                .setItems(streamLabels) { _, which ->
                    val chosen = streams[which]
                    val playIntent = Intent(this, PlayerActivity::class.java).apply {
                        putExtra(PlayerActivity.EXTRA_VIDEO_URI, chosen.url)
                        putExtra(PlayerActivity.EXTRA_VIDEO_TITLE, binding.tvWebTitle.text?.toString() ?: "Stream")
                    }
                    startActivity(playIntent)
                }
                .setNegativeButton("Cerrar", null)
                .show()
        }

        binding.btnWebPip.setOnClickListener {
            onPipRequested(fromUserTap = true)
        }

        onBackPressedDispatcher.addCallback(this, object : OnBackPressedCallback(true) {
            override fun handleOnBackPressed() {
                when {
                    customView != null -> {
                        binding.webViewPlayer.webChromeClient?.onHideCustomView()
                    }
                    binding.webViewPlayer.canGoBack() -> {
                        binding.webViewPlayer.goBack()
                    }
                    else -> {
                        finish()
                    }
                }
            }
        })
    }

    /**
     * Makes sure the page video keeps playing after a fullscreen exit.
     * Exiting the fullscreen custom view moves the video back to the inline
     * player and can pause it, which leaves a black frame. Play it again
     * through the KoalaSync bridge (YouTube API + native play, with a canplay
     * retry) and, as a safety net, directly on any paused video element
     * (in case the bridge script was not injected).
     */
    private fun resumeVideo() {
        webBridge.applyRemotePlay(null)
        binding.webViewPlayer.evaluateJavascript(
            "(function(){var yt=document.getElementById('movie_player');" +
                "if(yt&&typeof yt.playVideo==='function'){try{yt.playVideo();}catch(e){}}" +
                "var vs=document.querySelectorAll('video');" +
                "for(var i=0;i<vs.length;i++){if(vs[i].paused){var p=vs[i].play();" +
                "if(p&&typeof p.catch==='function'){p.catch(function(){});}}}})();",
            null
        )
    }

    override fun onUserLeaveHint() {
        super.onUserLeaveHint()
        if (webBridge.isVideoPlaying()) {
            if (currentYouTubeVideoId() == null || cachedYouTubeStreamUrl != null) {
                onPipRequested(fromUserTap = false)
            }
        }
    }

    override fun onPictureInPictureModeChanged(isInPictureInPictureMode: Boolean, newConfig: Configuration) {
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig)
        isInPipMode = isInPictureInPictureMode
        if (isInPictureInPictureMode) {
            binding.webTopBar.visibility = View.GONE
            // Do NOT hide the fullscreen custom view here: the playing video
            // surface must stay visible in the PiP window. Hiding it (as
            // v1.0.4/v1.0.5 did) destroys the surface and leaves PiP black.
            // The video keeps playing untouched.
        } else {
            // Back to full screen mode. Only restore the top bar when we are
            // not in fullscreen (the native custom view hides the top bar
            // itself while it is showing).
            if (customView == null) {
                binding.webTopBar.visibility = View.VISIBLE
            }
        }
    }

    private fun showKoalaSyncDialog() {
        val state = koalaSyncClient.roomState.value
        if (state.connectionState != KoalaSyncClient.ConnectionState.DISCONNECTED) {
            AlertDialog.Builder(this)
                .setTitle("KoalaSync")
                .setMessage(
                    "Sala: ${state.roomId}\n" +
                        "Participantes: ${state.peerCount}\n" +
                        "Modo: ${state.controlMode}"
                )
                .setPositiveButton("Desconectar") { _, _ ->
                    koalaSyncClient.disconnect()
                }
                .setNegativeButton("Cerrar", null)
                .show()
            return
        }

        val dialogBinding = DialogKoalasyncBinding.inflate(layoutInflater)
        if (prefs.defaultUsername.isNotBlank()) {
            dialogBinding.etUsername.setText(prefs.defaultUsername)
        }
        val dialog = AlertDialog.Builder(this)
            .setView(dialogBinding.root)
            .create()

        dialogBinding.btnCancelSync.setOnClickListener { dialog.dismiss() }
        dialogBinding.btnJoinSync.setOnClickListener {
            val roomId = dialogBinding.etRoomId.text?.toString()?.trim().orEmpty()
            if (roomId.isEmpty()) {
                dialogBinding.etRoomId.error = "Ingresa un ID de sala"
                return@setOnClickListener
            }
            val username = dialogBinding.etUsername.text?.toString()?.trim()?.ifEmpty { null }
            val password = dialogBinding.etRoomPassword.text?.toString()?.ifEmpty { null }
            val chatSecret = dialogBinding.etChatSecret.text?.toString()?.trim()?.ifEmpty { null }

            // Remember the credentials so a YouTube PiP handover can
            // rejoin the same room from the native player.
            activeRoomId = roomId
            activeRoomUsername = username
            activeRoomPassword = password
            activeRoomChatSecret = chatSecret

            koalaSyncClient.connect(
                roomId = roomId,
                username = username,
                password = password,
                chatSecret = chatSecret,
                mediaTitle = binding.tvWebTitle.text?.toString()
            )
            dialog.dismiss()
        }

        dialog.show()
    }

    private fun updateSnifferVisibility() {
        runOnUiThread {
            val hasStreams = com.alenia.mnemis.plugin.HlsSnifferPlugin.getDetectedStreams().isNotEmpty()
            binding.btnWebSniffer.visibility = if (hasStreams) View.VISIBLE else View.GONE
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        stopSnapshotPolling()
        PlaybackService.stop(this)
        koalaSyncClient.disconnect()
        binding.webViewPlayer.destroy()
    }
}
