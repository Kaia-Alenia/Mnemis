package com.alenia.mnemis.ui

import android.annotation.SuppressLint
import android.app.PictureInPictureParams
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.graphics.Bitmap
import android.os.Build
import android.os.Bundle
import android.util.Rational
import android.view.View
import android.webkit.JavascriptInterface
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
 * Fullscreen is handled IN THE PAGE (in-app fullscreen), not with the WebView
 * fullscreen custom view: the Fullscreen API is intercepted in JS
 * (see [injectFullscreenOverride]) so the video never leaves the inline
 * player. The native custom view moves the video onto a separate hardware
 * surface that PiP never captures -- that is why v1.0.4 and v1.0.5 showed a
 * black PiP window ("a black screen gets added"). With the video always
 * inline, PiP simply captures the normal activity window and the video
 * renders in it.
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
    private var isInAppFullscreen: Boolean = false
    private var pipExpandedVideo: Boolean = false

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

    /**
     * JS interface called by the injected Fullscreen API override when the
     * page enters or leaves in-app fullscreen. Runs on a background thread,
     * so it forwards to the UI thread.
     */
    private inner class FullscreenJsInterface {
        @JavascriptInterface
        fun onEnterFullscreen() {
            runOnUiThread { enterInAppFullscreen() }
        }

        @JavascriptInterface
        fun onExitFullscreen() {
            runOnUiThread { exitInAppFullscreen() }
        }
    }

    /**
     * Intercepts the Fullscreen API in the page so the video never enters
     * the native WebView fullscreen custom view.
     *
     * Why: the native custom view moves the video onto a separate hardware
     * surface. PiP captures the activity window, not that surface, so the
     * PiP window shows black (and the black fullscreen container covers the
     * page). Hiding the custom view first (v1.0.4) or entering PiP first and
     * hiding it after (v1.0.5) both failed, because the surface is gone
     * either way by the time PiP renders.
     *
     * Instead, requestFullscreen/exitFullscreen are replaced with an
     * in-page implementation: the player (YouTube #movie_player, or the
     * largest visible video) gets a fixed full-viewport class, the page is
     * told it is in fullscreen (fullscreenElement getter + fullscreenchange
     * event) so its fullscreen button shows the exit state, and the app is
     * notified through the MnemisFullscreen interface to hide the top bar
     * and rotate to landscape. The video stays in the inline player, so
     * PiP captures it normally.
     *
     * Idempotent (guarded by window.__mnemisFsOverride) and safe to inject
     * on every navigation. Exposes window.mnemisSetVideoFill(on) so the
     * app can expand the video just for PiP, and window.mnemisExitFullscreen()
     * for the Back button.
     */
    private fun injectFullscreenOverride() {
        val js = """
            (function(){
              if(window.__mnemisFsOverride){return;}
              window.__mnemisFsOverride=true;
              try{
                var st=document.createElement('style');
                st.id='mnemis-fs-style';
                st.textContent='.mnemis-video-fill{position:fixed!important;top:0!important;left:0!important;width:100vw!important;height:100vh!important;z-index:2147483647!important;background:#000!important;margin:0!important;padding:0!important;transform:none!important}.mnemis-video-fill video{width:100%!important;height:100%!important;object-fit:contain!important}';
                var h=document.head||document.getElementsByTagName('head')[0]||document.documentElement;
                if(h){h.appendChild(st);}
              }catch(e){}
              var fsEl=null;
              function findTarget(req){
                if(req&&(req.tagName==='VIDEO'||req.id==='movie_player')){return req;}
                var mp=document.getElementById('movie_player');
                if(mp){return mp;}
                var vs=document.querySelectorAll('video');
                var best=null,bestArea=0;
                for(var i=0;i<vs.length;i++){
                  var r=vs[i].getBoundingClientRect();
                  var a=r.width*r.height;
                  if(a>bestArea&&r.width>0&&r.height>0){bestArea=a;best=vs[i];}
                }
                return best||req;
              }
              window.mnemisSetVideoFill=function(on){
                if(on){
                  var t=findTarget(null);
                  if(!t){return false;}
                  fsEl=t;
                  t.classList.add('mnemis-video-fill');
                  return true;
                }
                if(fsEl){fsEl.classList.remove('mnemis-video-fill');fsEl=null;}
                var els=document.querySelectorAll('.mnemis-video-fill');
                for(var j=0;j<els.length;j++){els[j].classList.remove('mnemis-video-fill');}
                return true;
              };
              window.mnemisExitFullscreen=function(){
                window.mnemisSetVideoFill(false);
                try{document.dispatchEvent(new Event('fullscreenchange'));}catch(e){}
                try{if(window.MnemisFullscreen){MnemisFullscreen.onExitFullscreen();}}catch(e){}
              };
              function enterFs(req){
                var t=findTarget(req);
                if(!t){return Promise.reject(new Error('no video'));}
                fsEl=t;
                t.classList.add('mnemis-video-fill');
                try{document.dispatchEvent(new Event('fullscreenchange'));}catch(e){}
                try{if(window.MnemisFullscreen){MnemisFullscreen.onEnterFullscreen();}}catch(e){}
                return Promise.resolve();
              }
              function exitFs(){
                window.mnemisSetVideoFill(false);
                try{document.dispatchEvent(new Event('fullscreenchange'));}catch(e){}
                try{if(window.MnemisFullscreen){MnemisFullscreen.onExitFullscreen();}}catch(e){}
                return Promise.resolve();
              }
              try{
                Element.prototype.requestFullscreen=function(){return enterFs(this);};
                Element.prototype.webkitRequestFullscreen=function(){return enterFs(this);};
                Document.prototype.exitFullscreen=function(){return exitFs();};
                Document.prototype.webkitExitFullscreen=function(){return exitFs();};
              }catch(e){}
              try{
                Object.defineProperty(document,'fullscreenElement',{configurable:true,get:function(){return fsEl;}});
                Object.defineProperty(document,'webkitFullscreenElement',{configurable:true,get:function(){return fsEl;}});
              }catch(e){}
            })();
        """.trimIndent()
        binding.webViewPlayer.evaluateJavascript(js, null)
    }

    /**
     * Expands the video to fill the viewport (for PiP from inline mode) or
     * restores it. Uses the injected mnemisSetVideoFill; no-op if the
     * override script has not loaded yet.
     */
    private fun setVideoFill(expand: Boolean) {
        val js = if (expand) {
            "window.mnemisSetVideoFill && window.mnemisSetVideoFill(true);"
        } else {
            "window.mnemisSetVideoFill && window.mnemisSetVideoFill(false);"
        }
        binding.webViewPlayer.evaluateJavascript(js, null)
    }

    /**
     * App side of in-app fullscreen: hide the top bar and rotate to
     * landscape. Called from the JS interface when the page enters
     * fullscreen through the override.
     */
    private fun enterInAppFullscreen() {
        if (isInAppFullscreen) return
        isInAppFullscreen = true
        binding.webTopBar.visibility = View.GONE
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
    }

    /**
     * App side of leaving in-app fullscreen: restore the top bar (unless
     * in PiP) and the orientation.
     */
    private fun exitInAppFullscreen() {
        if (!isInAppFullscreen) return
        isInAppFullscreen = false
        if (!isInPipMode) {
            binding.webTopBar.visibility = View.VISIBLE
        }
        requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_UNSPECIFIED
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
            addJavascriptInterface(FullscreenJsInterface(), "MnemisFullscreen")

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
                    injectFullscreenOverride()
                }

                override fun onPageFinished(view: WebView?, loadedUrl: String?) {
                    super.onPageFinished(view, loadedUrl)
                    val activeUrl = loadedUrl ?: currentPageUrl
                    currentPageUrl = activeUrl
                    if (prefs.adBlockEnabled) {
                        webBridge.injectEarlyAdBlock()
                    }
                    injectMatchingWebPlugins(activeUrl)
                    injectFullscreenOverride()
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
                    injectFullscreenOverride()
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
                    // Primary path is the JS Fullscreen API override (in-app
                    // fullscreen): the page never enters native fullscreen, so
                    // this should not fire. If it does fire, the override did
                    // not load yet; fall back to the native custom view so
                    // the video is at least visible in fullscreen. (PiP from
                    // native fullscreen may still be black; the override is
                    // what fixes PiP.)
                    if (isInAppFullscreen) {
                        return
                    }
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
                    // If we are in JS in-app fullscreen, the JS exit path
                    // owns the restore; ignore the native callback.
                    if (isInAppFullscreen) {
                        return
                    }
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
                    isInAppFullscreen -> {
                        // Exit through the JS override so the page state
                        // (class, fullscreenElement, button state) is cleaned
                        // up and the app is notified through the interface.
                        binding.webViewPlayer.evaluateJavascript(
                            "window.mnemisExitFullscreen && window.mnemisExitFullscreen();",
                            null
                        )
                    }
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
     * The video is always inline now (in-app fullscreen keeps it in the
     * page; there is no native custom view in the primary path), so PiP
     * captures the normal activity window and the video renders in it.
     * When entering PiP from inline mode, the video is expanded to fill
     * the viewport first so it fills the PiP window instead of showing
     * small at the top of the page; the expansion is removed on PiP exit
     * (unless the user is in fullscreen).
     */
    private fun enterPipMode() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            Toast.makeText(this, "PiP no compatible con esta versión de Android", Toast.LENGTH_SHORT).show()
            return
        }
        if (isInPipMode) return
        if (!isInAppFullscreen && customView == null) {
            setVideoFill(true)
            pipExpandedVideo = true
        }
        val params = PictureInPictureParams.Builder()
            .setAspectRatio(Rational(16, 9))
            .build()
        val entered = enterPictureInPictureMode(params)
        if (!entered && customView != null) {
            // Fallback for the native fullscreen path only: exit the custom
            // view, make sure the video is playing again, and retry PiP once
            // the WebView has the inline video back. Only helps while the
            // activity is still in the foreground (e.g. button tap).
            binding.webViewPlayer.webChromeClient?.onHideCustomView()
            resumeVideo()
            binding.webViewPlayer.postDelayed({
                if (!isInPipMode && !isFinishing && !isDestroyed) {
                    enterPictureInPictureMode(params)
                }
            }, 350)
        }
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
        // Auto-PiP: works from in-app fullscreen too, because the video is
        // inline in the page (no custom view to hide) and enterPipMode
        // enters PiP immediately while the activity is still resumed.
        if (webBridge.isVideoPlaying()) {
            enterPipMode()
        }
    }

    override fun onPictureInPictureModeChanged(isInPictureInPictureMode: Boolean, newConfig: Configuration) {
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig)
        isInPipMode = isInPictureInPictureMode
        if (isInPictureInPictureMode) {
            binding.webTopBar.visibility = View.GONE
            // The video stays inline (in-app fullscreen or the PiP video
            // fill): nothing to hide and no surface to move. Just make sure
            // it keeps playing.
            resumeVideo()
        } else {
            // Back to full screen mode. Only restore the top bar when we are
            // not in fullscreen (in-app or native custom view, which hides
            // the top bar itself).
            if (!isInAppFullscreen && customView == null) {
                binding.webTopBar.visibility = View.VISIBLE
            }
            // The video was expanded only for PiP (not fullscreen): restore it.
            if (pipExpandedVideo && !isInAppFullscreen) {
                pipExpandedVideo = false
                setVideoFill(false)
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
