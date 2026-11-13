package com.alenia.mnemis.player

import android.content.Context
import android.net.Uri
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.MimeTypes
import androidx.media3.common.PlaybackException
import androidx.media3.common.Player
import androidx.media3.datasource.DefaultDataSource
import androidx.media3.datasource.okhttp.OkHttpDataSource
import androidx.media3.exoplayer.DefaultRenderersFactory
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory
import androidx.media3.ui.PlayerView
import okhttp3.OkHttpClient
import java.util.concurrent.TimeUnit

class MnemisPlayerManager(private val context: Context) {

    interface LocalPlaybackListener {
        fun onLocalPlay(positionMs: Long)
        fun onLocalPause(positionMs: Long)
        fun onLocalSeek(positionMs: Long)
    }

    var exoPlayer: ExoPlayer? = null
        private set

    private var localPlaybackListener: LocalPlaybackListener? = null
    private var suppressLocalEventsUntilMs: Long = 0L

    fun setLocalPlaybackListener(listener: LocalPlaybackListener?) {
        localPlaybackListener = listener
    }

    private val okHttpClient = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(20, TimeUnit.SECONDS)
        .followRedirects(true)
        .followSslRedirects(true)
        .build()

    fun initializePlayer(playerView: PlayerView, onPlayerError: (PlaybackException) -> Unit = {}) {
        if (exoPlayer != null) return

        // Configuramos el RenderersFactory con DECODER FALLBACK activado
        // Esto previene pantallas negras por desincronización de hardware en chipsets Mali / MediaTek
        val renderersFactory = DefaultRenderersFactory(context.applicationContext)
            .setEnableDecoderFallback(true)
            .setExtensionRendererMode(DefaultRenderersFactory.EXTENSION_RENDERER_MODE_PREFER)

        val httpDataSourceFactory = OkHttpDataSource.Factory(okHttpClient)
            .setUserAgent("Mozilla/5.0 (Linux; Android 13; Mobile) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Mobile Safari/537.36")
            .setDefaultRequestProperties(mapOf("Accept" to "*/*"))

        val dataSourceFactory = DefaultDataSource.Factory(context.applicationContext, httpDataSourceFactory)
        val mediaSourceFactory = DefaultMediaSourceFactory(dataSourceFactory)

        exoPlayer = ExoPlayer.Builder(context.applicationContext, renderersFactory)
            .setMediaSourceFactory(mediaSourceFactory)
            .setSeekBackIncrementMs(10000)
            .setSeekForwardIncrementMs(10000)
            .build()
            .apply {
                playWhenReady = true
                addListener(object : Player.Listener {
                    override fun onPlayerError(error: PlaybackException) {
                        onPlayerError(error)
                    }

                    override fun onPlayWhenReadyChanged(playWhenReady: Boolean, reason: Int) {
                        if (isLocalEventSuppressed()) return
                        if (reason != Player.PLAY_WHEN_READY_CHANGE_REASON_USER_REQUEST) return
                        val posMs = currentPosition.coerceAtLeast(0L)
                        if (playWhenReady) {
                            localPlaybackListener?.onLocalPlay(posMs)
                        } else {
                            localPlaybackListener?.onLocalPause(posMs)
                        }
                    }

                    override fun onPositionDiscontinuity(
                        oldPosition: Player.PositionInfo,
                        newPosition: Player.PositionInfo,
                        reason: Int
                    ) {
                        if (isLocalEventSuppressed()) return
                        if (reason == Player.DISCONTINUITY_REASON_SEEK) {
                            localPlaybackListener?.onLocalSeek(newPosition.positionMs.coerceAtLeast(0L))
                        }
                    }
                })
            }

        playerView.player = exoPlayer
    }

    fun playMedia(uri: Uri, mimeType: String? = null, title: String? = null) {
        val player = exoPlayer ?: return

        val builder = MediaItem.Builder().setUri(uri)

        // Asignación de MimeType según tipo o extensión
        val uriStr = uri.toString().lowercase()
        when {
            mimeType?.contains("mpegurl", ignoreCase = true) == true || uriStr.contains(".m3u8") -> {
                builder.setMimeType(MimeTypes.APPLICATION_M3U8)
            }
            mimeType?.contains("dash", ignoreCase = true) == true || uriStr.contains(".mpd") -> {
                builder.setMimeType(MimeTypes.APPLICATION_MPD)
            }
            else -> {
                builder.setMimeType(MimeTypes.VIDEO_MP4)
            }
        }

        if (!title.isNullOrEmpty()) {
            val mediaMetadata = androidx.media3.common.MediaMetadata.Builder()
                .setTitle(title)
                .build()
            builder.setMediaMetadata(mediaMetadata)
        }

        val mediaItem = builder.build()
        player.setMediaItem(mediaItem)
        player.prepare()
        player.play()
    }

    fun seekTo(positionMs: Long) {
        exoPlayer?.seekTo(positionMs)
    }

    fun seekBy(offsetMs: Long) {
        val player = exoPlayer ?: return
        val current = player.currentPosition
        val duration = player.duration
        val target = if (duration > 0) {
            (current + offsetMs).coerceIn(0L, duration)
        } else {
            (current + offsetMs).coerceAtLeast(0L)
        }
        player.seekTo(target)
    }

    fun togglePlayPause() {
        val player = exoPlayer ?: return
        if (player.isPlaying) {
            player.pause()
        } else {
            player.play()
        }
    }

    fun applyRemotePlay(targetTimeSeconds: Double?) {
        val player = exoPlayer ?: return
        suppressLocalEvents()
        if (targetTimeSeconds != null) {
            val targetMs = (targetTimeSeconds * 1000.0).toLong().coerceAtLeast(0L)
            if (kotlin.math.abs(player.currentPosition - targetMs) > REMOTE_DRIFT_THRESHOLD_MS) {
                player.seekTo(targetMs)
            }
        }
        player.play()
    }

    fun applyRemotePause(targetTimeSeconds: Double?) {
        val player = exoPlayer ?: return
        suppressLocalEvents()
        if (targetTimeSeconds != null) {
            val targetMs = (targetTimeSeconds * 1000.0).toLong().coerceAtLeast(0L)
            if (kotlin.math.abs(player.currentPosition - targetMs) > REMOTE_DRIFT_THRESHOLD_MS) {
                player.seekTo(targetMs)
            }
        }
        player.pause()
    }

    fun applyRemoteSeek(targetTimeSeconds: Double) {
        val player = exoPlayer ?: return
        suppressLocalEvents()
        val targetMs = (targetTimeSeconds * 1000.0).toLong().coerceAtLeast(0L)
        player.seekTo(targetMs)
    }

    fun correctDriftIfNeeded(expectedTimeSeconds: Double, thresholdMs: Long = DRIFT_CORRECTION_THRESHOLD_MS) {
        val player = exoPlayer ?: return
        if (!player.isPlaying) return
        val expectedMs = (expectedTimeSeconds * 1000.0).toLong().coerceAtLeast(0L)
        if (kotlin.math.abs(player.currentPosition - expectedMs) > thresholdMs) {
            suppressLocalEvents()
            player.seekTo(expectedMs)
        }
    }

    private fun suppressLocalEvents() {
        suppressLocalEventsUntilMs = System.currentTimeMillis() + REMOTE_SUPPRESS_WINDOW_MS
    }

    private fun isLocalEventSuppressed(): Boolean {
        return System.currentTimeMillis() < suppressLocalEventsUntilMs
    }

    fun release() {
        localPlaybackListener = null
        exoPlayer?.release()
        exoPlayer = null
    }

    companion object {
        private const val REMOTE_SUPPRESS_WINDOW_MS = 600L
        private const val REMOTE_DRIFT_THRESHOLD_MS = 400L
        private const val DRIFT_CORRECTION_THRESHOLD_MS = 1200L
    }
}

