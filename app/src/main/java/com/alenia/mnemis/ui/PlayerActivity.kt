package com.alenia.mnemis.ui

import android.app.PictureInPictureParams
import android.content.pm.ActivityInfo
import android.content.res.Configuration
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.util.Rational
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.view.WindowCompat
import androidx.core.view.WindowInsetsCompat
import androidx.core.view.WindowInsetsControllerCompat
import androidx.lifecycle.lifecycleScope
import androidx.media3.ui.AspectRatioFrameLayout
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.ActivityPlayerBinding
import com.alenia.mnemis.databinding.DialogKoalasyncBinding
import com.alenia.mnemis.player.MnemisPlayerManager
import com.alenia.mnemis.player.PlayerGestureHelper
import com.alenia.mnemis.prefs.MnemisPreferences
import com.alenia.mnemis.sync.KoalaSyncClient
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class PlayerActivity : AppCompatActivity(), PlayerGestureHelper.GestureListener {

    companion object {
        const val EXTRA_VIDEO_URI = "extra_video_uri"
        const val EXTRA_VIDEO_TITLE = "extra_video_title"
    }

    private lateinit var binding: ActivityPlayerBinding
    private lateinit var playerManager: MnemisPlayerManager
    private lateinit var gestureHelper: PlayerGestureHelper
    private lateinit var koalaSyncClient: KoalaSyncClient
    private lateinit var prefs: MnemisPreferences
    private var currentMediaTitle: String = "Reproduciendo"

    private val hideFeedbackHandler = Handler(Looper.getMainLooper())
    private val hideControlsHandler = Handler(Looper.getMainLooper())

    private var currentAspectRatioIndex = 0
    private val aspectRatios = intArrayOf(
        AspectRatioFrameLayout.RESIZE_MODE_FIT,
        AspectRatioFrameLayout.RESIZE_MODE_ZOOM,
        AspectRatioFrameLayout.RESIZE_MODE_FILL
    )
    private val aspectRatioNames = arrayOf("Ajustar", "Recortar (Zoom)", "Estirar")

    private var orientationMode = 0 // 0: Auto/Sensor, 1: Portrait, 2: Landscape

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityPlayerBinding.inflate(layoutInflater)
        setContentView(binding.root)

        keepScreenOn(true)
        hideSystemUI()

        val videoUriStr = intent.getStringExtra(EXTRA_VIDEO_URI)
        val videoTitle = intent.getStringExtra(EXTRA_VIDEO_TITLE) ?: "Reproduciendo"
        currentMediaTitle = videoTitle

        binding.tvPlayerTitle.text = videoTitle

        playerManager = MnemisPlayerManager(this)
        playerManager.initializePlayer(binding.playerView) { error ->
            Toast.makeText(this, "Error de reproducción: ${error.message}", Toast.LENGTH_LONG).show()
        }

        setupKoalaSync()
        setupControls()
        setupGestures()

        if (!videoUriStr.isNullOrEmpty()) {
            prefs.saveLastPlayed(videoTitle, videoUriStr, isWeb = false)
            playerManager.playMedia(Uri.parse(videoUriStr), title = videoTitle)
        } else {
            Toast.makeText(this, "No se proporcionó una URL de video válida", Toast.LENGTH_SHORT).show()
            finish()
        }
    }

    private fun setupControls() {
        binding.btnBack.setOnClickListener {
            finish()
        }

        binding.btnKoalaSync.setOnClickListener {
            showKoalaSyncDialog()
        }

        binding.btnAspectRatio.setOnClickListener {
            currentAspectRatioIndex = (currentAspectRatioIndex + 1) % aspectRatios.size
            binding.playerView.resizeMode = aspectRatios[currentAspectRatioIndex]
            Toast.makeText(this, "Modo: ${aspectRatioNames[currentAspectRatioIndex]}", Toast.LENGTH_SHORT).show()
        }

        binding.btnRotation.setOnClickListener {
            cycleOrientation()
        }

        binding.btnPip.setOnClickListener {
            enterPipMode()
        }
    }

    private fun setupGestures() {
        gestureHelper = PlayerGestureHelper(this, playerManager, this)
        binding.gestureTouchView.setOnTouchListener(gestureHelper)
    }

    private fun cycleOrientation() {
        orientationMode = (orientationMode + 1) % 3
        when (orientationMode) {
            0 -> {
                requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR
                Toast.makeText(this, "Rotación: Automática por sensor", Toast.LENGTH_SHORT).show()
            }
            1 -> {
                requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT
                Toast.makeText(this, "Rotación: Vertical (Portrait)", Toast.LENGTH_SHORT).show()
            }
            2 -> {
                requestedOrientation = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE
                Toast.makeText(this, "Rotación: Horizontal (Landscape)", Toast.LENGTH_SHORT).show()
            }
        }
    }

    private fun enterPipMode() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val rational = Rational(16, 9)
            val params = PictureInPictureParams.Builder()
                .setAspectRatio(rational)
                .build()
            enterPictureInPictureMode(params)
        } else {
            Toast.makeText(this, "PiP no compatible con esta versión de Android", Toast.LENGTH_SHORT).show()
        }
    }

    override fun onUserLeaveHint() {
        super.onUserLeaveHint()
        if (playerManager.exoPlayer?.isPlaying == true) {
            enterPipMode()
        }
    }

    override fun onPictureInPictureModeChanged(isInPictureInPictureMode: Boolean, newConfig: Configuration) {
        super.onPictureInPictureModeChanged(isInPictureInPictureMode, newConfig)
        if (isInPictureInPictureMode) {
            binding.topBar.visibility = View.GONE
            binding.layoutGestureFeedback.visibility = View.GONE
            binding.playerView.useController = false
        } else {
            binding.topBar.visibility = View.VISIBLE
            binding.playerView.useController = true
            hideSystemUI()
        }
    }

    // --- Implementación de GestureListener ---

    override fun onBrightnessChanged(percent: Int) {
        showFeedback(R.drawable.ic_brightness, "Brillo: $percent%", percent)
    }

    override fun onVolumeChanged(percent: Int) {
        showFeedback(R.drawable.ic_volume, "Volumen: $percent%", percent)
    }

    override fun onSeekPreview(seekOffsetMs: Long, targetTimeMs: Long) {
        val sign = if (seekOffsetMs >= 0) "+" else ""
        val offsetSec = seekOffsetMs / 1000
        val targetSec = targetTimeMs / 1000
        val text = "$sign${offsetSec}s (${targetSec / 60}:${String.format("%02d", targetSec % 60)})"
        showFeedback(R.drawable.ic_play_arrow, text, null)
    }

    override fun onSeekCommit(targetTimeMs: Long) {
        playerManager.seekTo(targetTimeMs)
        hideFeedback()
    }

    override fun onGestureEnd() {
        hideFeedbackHandler.removeCallbacksAndMessages(null)
        hideFeedbackHandler.postDelayed({
            hideFeedback()
        }, 800)
    }

    override fun onSingleTap() {
        if (binding.topBar.visibility == View.VISIBLE) {
            binding.topBar.visibility = View.GONE
            binding.playerView.hideController()
        } else {
            binding.topBar.visibility = View.VISIBLE
            binding.playerView.showController()
            hideControlsHandler.removeCallbacksAndMessages(null)
            hideControlsHandler.postDelayed({
                binding.topBar.visibility = View.GONE
            }, 4000)
        }
    }

    private fun showFeedback(iconRes: Int, text: String, progress: Int?) {
        hideFeedbackHandler.removeCallbacksAndMessages(null)
        binding.ivFeedbackIcon.setImageResource(iconRes)
        binding.tvFeedbackText.text = text

        if (progress != null) {
            binding.pbFeedbackProgress.visibility = View.VISIBLE
            binding.pbFeedbackProgress.progress = progress
        } else {
            binding.pbFeedbackProgress.visibility = View.GONE
        }

        binding.layoutGestureFeedback.visibility = View.VISIBLE
    }

    private fun hideFeedback() {
        binding.layoutGestureFeedback.visibility = View.GONE
    }

    private fun keepScreenOn(keep: Boolean) {
        if (keep) {
            window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        } else {
            window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
    }

    private fun hideSystemUI() {
        WindowCompat.setDecorFitsSystemWindows(window, false)
        val controller = WindowInsetsControllerCompat(window, window.decorView)
        controller.hide(WindowInsetsCompat.Type.systemBars())
        controller.systemBarsBehavior = WindowInsetsControllerCompat.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE
    }

    override fun onResume() {
        super.onResume()
        hideSystemUI()
    }

    private fun setupKoalaSync() {
        prefs = MnemisPreferences(this)
        binding.btnKoalaSync.visibility = if (prefs.koalaSyncPluginEnabled) View.VISIBLE else View.GONE
        koalaSyncClient = KoalaSyncClient(
            serverUrl = prefs.relayServerUrl,
            listener = object : KoalaSyncClient.SyncListener {
                override fun onRemotePlay(senderId: String, currentTimeSeconds: Double?) {
                    runOnUiThread {
                        playerManager.applyRemotePlay(currentTimeSeconds)
                    }
                }

                override fun onRemotePause(senderId: String, currentTimeSeconds: Double?) {
                    runOnUiThread {
                        playerManager.applyRemotePause(currentTimeSeconds)
                    }
                }

                override fun onRemoteSeek(senderId: String, targetTimeSeconds: Double) {
                    runOnUiThread {
                        playerManager.applyRemoteSeek(targetTimeSeconds)
                    }
                }

                override fun onRemoteDriftCheck(senderId: String, expectedTimeSeconds: Double) {
                    runOnUiThread {
                        playerManager.correctDriftIfNeeded(expectedTimeSeconds)
                    }
                }

                override fun onSyncError(message: String) {
                    runOnUiThread {
                        Toast.makeText(this@PlayerActivity, "KoalaSync: $message", Toast.LENGTH_LONG).show()
                    }
                }
            }
        )

        koalaSyncClient.setSnapshotProvider {
            withContext(Dispatchers.Main) {
                val player = playerManager.exoPlayer
                KoalaSyncClient.PlaybackSnapshot(
                    isPlaying = player?.isPlaying == true,
                    currentTimeSeconds = (player?.currentPosition ?: 0L).coerceAtLeast(0L) / 1000.0,
                    mediaTitle = currentMediaTitle
                )
            }
        }

        playerManager.setLocalPlaybackListener(object : MnemisPlayerManager.LocalPlaybackListener {
            override fun onLocalPlay(positionMs: Long) {
                koalaSyncClient.sendPlay(positionMs / 1000.0)
            }

            override fun onLocalPause(positionMs: Long) {
                koalaSyncClient.sendPause(positionMs / 1000.0)
            }

            override fun onLocalSeek(positionMs: Long) {
                koalaSyncClient.sendSeek(positionMs / 1000.0)
            }
        })

        binding.btnPlayerChat.setOnClickListener {
            KoalaSyncChatDialogHelper.show(this, lifecycleScope, koalaSyncClient)
        }

        lifecycleScope.launch {
            koalaSyncClient.roomState.collect { state ->
                val tintColorRes = when (state.connectionState) {
                    KoalaSyncClient.ConnectionState.CONNECTED -> R.color.teal_200
                    KoalaSyncClient.ConnectionState.CONNECTING -> R.color.accent_indicator
                    KoalaSyncClient.ConnectionState.DISCONNECTED -> R.color.white
                }
                binding.btnKoalaSync.setColorFilter(
                    ContextCompat.getColor(this@PlayerActivity, tintColorRes)
                )
                binding.btnPlayerChat.visibility = if (state.connectionState == KoalaSyncClient.ConnectionState.CONNECTED) {
                    View.VISIBLE
                } else {
                    View.GONE
                }
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

        dialogBinding.btnCancelSync.setOnClickListener {
            dialog.dismiss()
        }

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
                mediaTitle = currentMediaTitle
            )
            dialog.dismiss()
        }

        dialog.show()
    }

    override fun onDestroy() {
        super.onDestroy()
        hideFeedbackHandler.removeCallbacksAndMessages(null)
        hideControlsHandler.removeCallbacksAndMessages(null)
        koalaSyncClient.disconnect()
        playerManager.release()
    }
}

