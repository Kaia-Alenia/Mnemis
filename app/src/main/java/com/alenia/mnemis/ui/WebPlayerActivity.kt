package com.alenia.mnemis.ui

import android.annotation.SuppressLint
import android.graphics.Bitmap
import android.os.Bundle
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
import com.alenia.mnemis.prefs.MnemisPreferences
import com.alenia.mnemis.sync.KoalaSyncClient
import com.alenia.mnemis.sync.KoalaSyncWebBridge
import com.alenia.mnemis.sync.WebAdBlocker
import kotlinx.coroutines.launch

/**
 * WebView-based player with injected KoalaSync plugin script for web streaming platforms.
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
                    binding.webTopBar.visibility = View.VISIBLE
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
        koalaSyncClient.disconnect()
        binding.webViewPlayer.destroy()
    }
}

