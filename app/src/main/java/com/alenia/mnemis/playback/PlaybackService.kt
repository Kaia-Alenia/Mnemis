package com.alenia.mnemis.playback

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import androidx.core.app.NotificationCompat
import com.alenia.mnemis.R
import com.alenia.mnemis.player.MnemisPlayerManager
import com.alenia.mnemis.ui.MainActivity

/**
 * Foreground service that keeps playback and the KoalaSync room connection
 * alive while the app is in the background (user leaves the app or turns
 * the screen off). Shows a media notification with play/pause and stop actions.
 */
class PlaybackService : Service() {

    companion object {
        const val ACTION_START = "com.alenia.mnemis.playback.START"
        const val ACTION_STOP = "com.alenia.mnemis.playback.STOP"
        const val ACTION_PLAY_PAUSE = "com.alenia.mnemis.playback.PLAY_PAUSE"
        const val EXTRA_TITLE = "extra_title"
        private const val NOTIFICATION_ID = 1001
        private const val CHANNEL_ID = "mnemis_playback"

        fun start(context: Context, title: String?) {
            val intent = Intent(context, PlaybackService::class.java).apply {
                action = ACTION_START
                putExtra(EXTRA_TITLE, title)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                context.startForegroundService(intent)
            } else {
                context.startService(intent)
            }
        }

        fun stop(context: Context) {
            val intent = Intent(context, PlaybackService::class.java).apply {
                action = ACTION_STOP
            }
            context.startService(intent)
        }
    }

    private var wakeLock: PowerManager.WakeLock? = null
    private var currentTitle: String? = null

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        createNotificationChannel()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> {
                MnemisPlayerManager.activePlayerManager?.exoPlayer?.pause()
                stopForeground(STOP_FOREGROUND_REMOVE)
                stopSelf()
                return START_NOT_STICKY
            }
            ACTION_PLAY_PAUSE -> {
                MnemisPlayerManager.activePlayerManager?.togglePlayPause()
                val isPlaying = MnemisPlayerManager.activePlayerManager?.exoPlayer?.isPlaying == true
                val notificationManager = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
                notificationManager.notify(NOTIFICATION_ID, buildNotification(isPlaying))
                return START_NOT_STICKY
            }
            else -> {
                currentTitle = intent?.getStringExtra(EXTRA_TITLE) ?: currentTitle
                acquireWakeLock()
                val isPlaying = MnemisPlayerManager.activePlayerManager?.exoPlayer?.isPlaying == true
                val notification = buildNotification(isPlaying)
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
                    startForeground(NOTIFICATION_ID, notification, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK)
                } else {
                    startForeground(NOTIFICATION_ID, notification)
                }
                return START_NOT_STICKY
            }
        }
    }

    override fun onDestroy() {
        releaseWakeLock()
        super.onDestroy()
    }

    private fun acquireWakeLock() {
        if (wakeLock?.isHeld == true) return
        val powerManager = getSystemService(POWER_SERVICE) as PowerManager
        wakeLock = powerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "Mnemis:PlaybackKeepAlive"
        ).apply { acquire() }
    }

    private fun releaseWakeLock() {
        wakeLock?.let { if (it.isHeld) it.release() }
        wakeLock = null
    }

    private fun createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val channel = NotificationChannel(
                CHANNEL_ID,
                getString(R.string.playback_channel_name),
                NotificationManager.IMPORTANCE_LOW
            )
            (getSystemService(NOTIFICATION_SERVICE) as NotificationManager)
                .createNotificationChannel(channel)
        }
    }

    private fun buildNotification(isPlaying: Boolean = true): Notification {
        val openIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val stopIntent = PendingIntent.getService(
            this,
            1,
            Intent(this, PlaybackService::class.java).apply { action = ACTION_STOP },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val playPauseIntent = PendingIntent.getService(
            this,
            2,
            Intent(this, PlaybackService::class.java).apply { action = ACTION_PLAY_PAUSE },
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val playPauseIcon = if (isPlaying) R.drawable.ic_pause else R.drawable.ic_play_arrow
        val playPauseLabel = if (isPlaying) "Pausar" else "Reproducir"

        return NotificationCompat.Builder(this, CHANNEL_ID)
            .setSmallIcon(R.drawable.ic_play_arrow)
            .setContentTitle(currentTitle ?: getString(R.string.app_name))
            .setContentText(getString(R.string.playback_notification_text))
            .setContentIntent(openIntent)
            .addAction(playPauseIcon, playPauseLabel, playPauseIntent)
            .addAction(0, getString(R.string.playback_stop), stopIntent)
            .setOngoing(isPlaying)
            .setOnlyAlertOnce(true)
            .build()
    }
}
