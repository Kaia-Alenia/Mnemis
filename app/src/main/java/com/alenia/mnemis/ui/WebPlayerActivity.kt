package com.alenia.mnemis.ui

import android.annotation.SuppressLint
import android.app.PictureInPictureParams
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
import com.alenia.mnemis.sync.KoalaSyncClient
import com.alenia.mnemis.sync.KoalaSyncWebBridge
import com.alenia.mnemis.sync.WebAdBlocker
import kotlinx.coroutines.launch

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
 * PiP: when entering PiP the fullscreen custom view is LEFT SHOWING.
 * v1.0.4 hid it first and v1.0.5 hid it after entering PiP; both destroyed
 * the video surface and left the PiP window black. Leaving the custom view
 * alone keeps the playing video surface in the PiP window; on PiP exit the
 * user is still in fullscreen, which is seamless. The video surface is
 * only torn down when the user explicitly exits fullscreen (Back button or
 * the player's exit control), and then [resumeVideo] restarts inline
 * playback.
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

    private fun setupSyncAndBridge() {
        prefs = MnemisPreferences(this)
        binding.btnWebKoalaSync.visibility = if (prefs.koalaSyncPluginEnabled) View.VISIBLE else View.GONE
        koalaSyncClient = KoalaSyncClient(
            serverUrl = prefs.relayServerUrl,
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
                    if (prefs.adBlockEnabled) {
                        webBridge.injectEarlyAdBlock()
                    }
                }

                override fun onPageFinished(view: WebView?, loadedUrl: String?) {
                    super.onPageFinished(view, loadedUrl)
                    val activeUrl = loadedUrl ?: currentPageUrl
                    currentPageUrl = activeUrl
                    if (prefs.adBlockEnabled) {
                        webBridge.injectEarlyAdBlock()
                    }
                    injectMatchingWebPlugins(activeUrl)
                    if (prefs.koalaSyncPluginEnabled) {
                        webBridge.injectScript()
                    }
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
                    injectMatchingWebPlugins(activeUrl)
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

        binding.btnWebPip.setOnClickListener {
            enterPipMode()
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
     * Enters picture-in-picture.
     *
     * The fullscreen custom view is deliberately LEFT SHOWING when entering
     * PiP. v1.0.4 hid it first and v1.0.5 hid it after entering PiP; both
     * destroyed the video surface and left the PiP window black. Leaving
     * the custom view alone keeps the playing video surface in the PiP
     * window. On PiP exit the user is still in fullscreen, which is
     * seamless -- Back or the player's exit control leaves fullscreen and
     * [resumeVideo] restarts inline playback. From inline mode there is
     * nothing to hide either: PiP captures the activity window with the
     * video in the page.
     */
    private fun enterPipMode() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            Toast.makeText(this, "PiP no compatible con esta versión de Android", Toast.LENGTH_SHORT).show()
            return
        }
        if (isInPipMode) return
        val params = PictureInPictureParams.Builder()
            .setAspectRatio(Rational(16, 9))
            .build()
        enterPictureInPictureMode(params)
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
        // Auto-PiP: works from native fullscreen too, because enterPipMode
        // leaves the fullscreen custom view showing and enters PiP
        // immediately while the activity is still resumed.
        if (webBridge.isVideoPlaying()) {
            enterPipMode()
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

    override fun onDestroy() {
        super.onDestroy()
        PlaybackService.stop(this)
        koalaSyncClient.disconnect()
        binding.webViewPlayer.destroy()
    }
}
