package com.alenia.mnemis.player

import android.app.Activity
import android.content.Context
import android.media.AudioManager
import android.view.GestureDetector
import android.view.MotionEvent
import android.view.View
import android.view.WindowManager
import kotlin.math.abs

class PlayerGestureHelper(
    private val activity: Activity,
    private val playerManager: MnemisPlayerManager,
    private val listener: GestureListener
) : View.OnTouchListener {

    interface GestureListener {
        fun onBrightnessChanged(percent: Int)
        fun onVolumeChanged(percent: Int)
        fun onSeekPreview(seekOffsetMs: Long, targetTimeMs: Long)
        fun onSeekCommit(targetTimeMs: Long)
        fun onGestureEnd()
        fun onSingleTap()
    }

    private val audioManager = activity.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    private val maxVolume = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC)

    private var screenWidth = 0
    private var screenHeight = 0

    private var initialBrightness = 0.5f
    private var initialVolume = 0

    private var isDragging = false
    private var dragMode = DragMode.NONE
    private var seekTargetMs = 0L

    private enum class DragMode {
        NONE, BRIGHTNESS, VOLUME, SEEK
    }

    private val gestureDetector = GestureDetector(activity, object : GestureDetector.SimpleOnGestureListener() {
        override fun onSingleTapConfirmed(e: MotionEvent): Boolean {
            listener.onSingleTap()
            return true
        }

        override fun onDoubleTap(e: MotionEvent): Boolean {
            val width = screenWidth.toFloat()
            val x = e.x

            when {
                x < width * 0.35f -> {
                    // Doble toque a la izquierda: -10 segundos
                    playerManager.seekBy(-10000)
                }
                x > width * 0.65f -> {
                    // Doble toque a la derecha: +10 segundos
                    playerManager.seekBy(10000)
                }
                else -> {
                    // Doble toque en el centro: alternar play/pause
                    playerManager.togglePlayPause()
                }
            }
            return true
        }
    })

    override fun onTouch(v: View, event: MotionEvent): Boolean {
        screenWidth = v.width
        screenHeight = v.height

        if (gestureDetector.onTouchEvent(event)) {
            return true
        }

        when (event.action) {
            MotionEvent.ACTION_DOWN -> {
                isDragging = false
                dragMode = DragMode.NONE

                // Obtener brillo actual
                val lp = activity.window.attributes
                initialBrightness = if (lp.screenBrightness < 0) 0.5f else lp.screenBrightness

                // Obtener volumen actual
                initialVolume = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC)
            }

            MotionEvent.ACTION_MOVE -> {
                val dx = event.x - (event.historySize.takeIf { it > 0 }?.let { event.getHistoricalX(0) } ?: event.x)
                val dy = event.y - (event.historySize.takeIf { it > 0 }?.let { event.getHistoricalY(0) } ?: event.y)

                if (!isDragging) {
                    if (abs(dx) > 10 || abs(dy) > 10) {
                        isDragging = true
                        dragMode = if (abs(dx) > abs(dy)) {
                            DragMode.SEEK
                        } else {
                            if (event.x < screenWidth / 2f) DragMode.BRIGHTNESS else DragMode.VOLUME
                        }
                    }
                }

                if (isDragging) {
                    when (dragMode) {
                        DragMode.BRIGHTNESS -> handleBrightnessDrag(event)
                        DragMode.VOLUME -> handleVolumeDrag(event)
                        DragMode.SEEK -> handleSeekDrag(event)
                        DragMode.NONE -> {}
                    }
                }
            }

            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                if (isDragging && dragMode == DragMode.SEEK) {
                    listener.onSeekCommit(seekTargetMs)
                }
                isDragging = false
                dragMode = DragMode.NONE
                listener.onGestureEnd()
            }
        }

        return true
    }

    private var touchStartY = 0f
    private var touchStartX = 0f

    private fun handleBrightnessDrag(event: MotionEvent) {
        val deltaRatio = -(event.y - screenHeight / 2f) / (screenHeight / 2f)
        val newBrightness = (initialBrightness + deltaRatio * 0.15f).coerceIn(0.01f, 1.0f)

        val lp = activity.window.attributes
        lp.screenBrightness = newBrightness
        activity.window.attributes = lp

        listener.onBrightnessChanged((newBrightness * 100).toInt())
    }

    private fun handleVolumeDrag(event: MotionEvent) {
        val deltaY = -(event.y - screenHeight / 2f)
        val step = (deltaY / (screenHeight / 4f) * maxVolume).toInt()
        val newVolume = (initialVolume + step).coerceIn(0, maxVolume)

        audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, newVolume, 0)
        val percent = ((newVolume.toFloat() / maxVolume.toFloat()) * 100).toInt()
        listener.onVolumeChanged(percent)
    }

    private fun handleSeekDrag(event: MotionEvent) {
        val player = playerManager.exoPlayer ?: return
        val duration = player.duration
        if (duration <= 0) return

        val deltaX = event.x - (screenWidth / 2f)
        val ratio = deltaX / (screenWidth / 2f)
        val offsetMs = (ratio * 60000).toLong() // +/- hasta 60s en un deslizamiento medio

        val currentPosition = player.currentPosition
        seekTargetMs = (currentPosition + offsetMs).coerceIn(0L, duration)

        listener.onSeekPreview(offsetMs, seekTargetMs)
    }
}

