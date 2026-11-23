package com.alenia.mnemis.ui

import android.annotation.SuppressLint
import android.content.Context
import android.content.Intent
import android.content.pm.ActivityInfo
import android.graphics.Bitmap
import android.os.Bundle
import android.util.AttributeSet
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.webkit.WebChromeClient
import android.webkit.WebResourceRequest
import android.webkit.WebResourceResponse
import android.webkit.WebSettings
import android.webkit.WebView
import android.webkit.WebViewClient
import android.widget.FrameLayout
import android.widget.Toast
import androidx.activity.OnBackPressedCallback
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.lifecycleScope
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.ActivityWebPlayerBinding
import com.alenia.mnemis.databinding.DialogKoalasyncBinding
import com.alenia.mnemis.playback.PlaybackService
import com.alenia.mnemis.plugin.DynamicPluginManager
import com.alenia.mnemis.plugin.MnemisPluginRegistry
import com.alenia.mnemis.prefs.MnemisPreferences
import com.alenia.mnemis.sync.KoalaSyncClient
import com.alenia.mnemis.sync.KoalaSyncProtocol
import com.alenia.mnemis.sync.KoalaSyncWebBridge
import com.alenia.mnemis.sync.WebAdBlocker
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

/**
 * Reproductor web basado en WebView con integración nativa de KoalaSync,
 * pantalla completa inmersiva en horizontal (landscape) y reproducción
 * continua en segundo plano sin cierre al minimizar o apagar la pantalla.
 */
class WebPlayerActivity : AppCompatActivity() {

    companion object {
        const val EXTRA_WEB_URL = "extra_web_url"
        const val EXTRA_WEB_TITLE = "extra_web_title"

        private const val BG_PLAYBACK_JS =
            "(function(){" +
                "if(window.__mnemisBgPlayPatched)return;" +
                "window.__mnemisBgPlayPatched=true;" +
                "try{" +
                "Object.defineProperty(document,'hidden',{get:function(){return false;},configurable:true});" +
                "Object.defineProperty(document,'visibilityState',{get:function(){return 'visible';},configurable:true});" +
                "window.addEventListener('visibilitychange',function(e){e.stopImmediatePropagation();},true);" +
                "document.addEventListener('visibilitychange',function(e){e.stopImmediatePropagation();},true);" +
                "}catch(e){}" +
                "})();"
    }

    private lateinit var binding: ActivityWebPlayerBinding
    private lateinit var koalaSyncClient: KoalaSyncClient
    private lateinit var webBridge: KoalaSyncWebBridge
    private lateinit var prefs: MnemisPreferences
    private var customView: View? = null
    private var customViewCallback: WebChromeClient.CustomViewCallback? = null
    private var currentPageUrl: String = ""
    private var lastBroadcastedMediaKey: String = ""
    private var playbackMonitorJob: Job? = null

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
        if (customView != null) {
            hideSystemUI()
        }
        startPlaybackMonitor()
    }

    override fun onPause() {
        // Mantener PlaybackService vivo si el video web está reproduciéndose o hay sala activa
        updatePlaybackService()
        super.onPause()
    }

    override fun onStop() {
        updatePlaybackService()
        super.onStop()
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
                        updatePlaybackService()
                    }
                }

                override fun onRemotePause(senderId: String, currentTimeSeconds: Double?) {
                    runOnUiThread {
                        webBridge.applyRemotePause(currentTimeSeconds)
                        updatePlaybackService()
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

                override fun onRemoteMediaUrlChanged(senderId: String, mediaUrl: String) {
                    if (!KoalaSyncProtocol.isSafeWebMediaUrl(mediaUrl)) return
                    runOnUiThread {
                        val incomingKey = KoalaSyncProtocol.normalizeMediaIdentity(mediaUrl)
                        val currentKey = KoalaSyncProtocol.normalizeMediaIdentity(currentPageUrl)
                        if (incomingKey.isEmpty() || incomingKey == currentKey) return@runOnUiThread
                        currentPageUrl = mediaUrl
                        lastBroadcastedMediaKey = incomingKey
                        Toast.makeText(
                            this@WebPlayerActivity,
                            "Sincronizando video del líder...",
                            Toast.LENGTH_SHORT
                        ).show()
                        binding.webViewPlayer.loadUrl(mediaUrl)
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
            onRunOnUiThread = { action -> runOnUiThread(action) },
            onUrlChanged = { newUrl ->
                runOnUiThread {
                    handleLocalUrlChanged(newUrl)
                }
            }
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
                if (state.connectionState == KoalaSyncClient.ConnectionState.CONNECTED && state.isHost) {
                    broadcastCurrentMediaIfHost(force = true)
                } else if (state.connectionState == KoalaSyncClient.ConnectionState.DISCONNECTED) {
                    lastBroadcastedMediaKey = ""
                }
                updatePlaybackService()
            }
        }
    }

    private fun handleLocalUrlChanged(newUrl: String) {
        if (newUrl.isBlank()) return
        currentPageUrl = newUrl
        broadcastCurrentMediaIfHost(force = false)
    }

    private fun broadcastCurrentMediaIfHost(force: Boolean) {
        if (!koalaSyncClient.isHost()) return
        val url = currentPageUrl
        if (!KoalaSyncProtocol.isSafeWebMediaUrl(url)) return
        val mediaKey = KoalaSyncProtocol.normalizeMediaIdentity(url)
        if (mediaKey.isEmpty()) return
        if (!force && mediaKey == lastBroadcastedMediaKey) return
        if (koalaSyncClient.broadcastMediaUrl(url)) {
            lastBroadcastedMediaKey = mediaKey
        }
    }

    private fun startPlaybackMonitor() {
        playbackMonitorJob?.cancel()
        playbackMonitorJob = lifecycleScope.launch {
            while (isActive) {
                delay(2500)
                updatePlaybackService()
            }
        }
    }

    private fun updatePlaybackService() {
        val isPlaying = webBridge.isVideoPlaying()
        val isConnected = koalaSyncClient.roomState.value.connectionState ==
            KoalaSyncClient.ConnectionState.CONNECTED
        if (isPlaying || isConnected) {
            PlaybackService.start(this, binding.tvWebTitle.text?.toString())
        } else {
            PlaybackService.stop(this)
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

    private fun injectBackgroundPlaybackPatch() {
        binding.webViewPlayer.evaluateJavascript(BG_PLAYBACK_JS, null)
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
                    injectBackgroundPlaybackPatch()
                    if (prefs.adBlockEnabled) {
                        webBridge.injectEarlyAdBlock()
                    }
                }

                override fun onPageFinished(view: WebView?, loadedUrl: String?) {
                    super.onPageFinished(view, loadedUrl)
                    val activeUrl = loadedUrl ?: currentPageUrl
                    handleLocalUrlChanged(activeUrl)
                    injectBackgroundPlaybackPatch()
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
                    handleLocalUrlChanged(activeUrl)
                    injectBackgroundPlaybackPatch()
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
                    if (customView != null) {
                        callback?.onCustomViewHidden()
                        return
                    }
                    customView = view
                    customViewCallback = callback

                    // Pasar a horizontal inmersivo perfecto (como YouTube nativo)
                    requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
                    window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
                    hideSystemUI()

                    binding.fullscreenContainer.removeAllViews()
                    if (view != null) {
                        binding.fullscreenContainer.addView(
                            view,
                            FrameLayout.LayoutParams(
                                ViewGroup.LayoutParams.MATCH_PARENT,
                                ViewGroup.LayoutParams.MATCH_PARENT
                            )
                        )
                    }
                    binding.fullscreenContainer.visibility = View.VISIBLE
                    binding.webContentContainer.visibility = View.GONE
                }

                override fun onHideCustomView() {
                    binding.fullscreenContainer.removeAllViews()
                    binding.fullscreenContainer.visibility = View.GONE
                    binding.webContentContainer.visibility = View.VISIBLE

                    // Restaurar orientación normal y barras del sistema
                    requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
                    window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
                    showSystemUI()

                    customView = null
                    customViewCallback?.onCustomViewHidden()
                    customViewCallback = null

                    // Mantener la reproducción activa al salir de pantalla completa
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

    private fun hideSystemUI() {
        WindowCompat.setDecorFitsSystemWindows(window, false)
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }

    private fun showSystemUI() {
        WindowCompat.setDecorFitsSystemWindows(window, true)
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        controller.show(WindowInsetsCompat.Type.systemBars())
    }

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

    private fun showKoalaSyncDialog() {
        val state = koalaSyncClient.roomState.value
        if (state.connectionState != KoalaSyncClient.ConnectionState.DISCONNECTED) {
            AlertDialog.Builder(this)
                .setTitle("KoalaSync")
                .setMessage(
                    "Sala: ${state.roomId}\n" +
                        "Rol: ${if (state.isHost) "Líder" else "Invitado"}\n" +
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

    private fun updateSnifferVisibility() {
        runOnUiThread {
            val hasStreams = com.alenia.mnemis.plugin.HlsSnifferPlugin.getDetectedStreams().isNotEmpty()
            binding.btnWebSniffer.visibility = if (hasStreams) View.VISIBLE else View.GONE
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        playbackMonitorJob?.cancel()
        PlaybackService.stop(this)
        koalaSyncClient.disconnect()
        binding.webViewPlayer.destroy()
    }
}

/**
 * WebView que mantiene su visibilidad interna activa cuando la ventana pasa a segundo plano,
 * evitando que Chromium pause el audio de YouTube al minimizar la app o apagar la pantalla.
 */
class BackgroundWebView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = android.R.attr.webViewStyle
) : WebView(context, attrs, defStyleAttr) {

    override fun onWindowVisibilityChanged(visibility: Int) {
        if (visibility != View.GONE && visibility != View.INVISIBLE) {
            super.onWindowVisibilityChanged(visibility)
        } else {
            super.onWindowVisibilityChanged(View.VISIBLE)
        }
    }
}
