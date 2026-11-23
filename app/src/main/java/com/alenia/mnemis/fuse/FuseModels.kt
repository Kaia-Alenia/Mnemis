package com.alenia.mnemis.fuse

import android.net.Uri
import java.util.Locale

/**
 * Technical inspection details for a media file, modelled after Alenia Fuse Media model.
 */
data class FuseMediaInfo(
    val uri: Uri? = null,
    val name: String,
    val path: String? = null,
    val durationMs: Long = 0L,
    val sizeBytes: Long = 0L,
    val width: Int = 0,
    val height: Int = 0,
    val videoMime: String? = null,
    val audioMime: String? = null,
    val videoBitrate: Int = 0,
    val audioBitrate: Int = 0,
    val frameRate: Float = 0f,
    val channelCount: Int = 0,
    val sampleRate: Int = 0,
    val rotation: Int = 0
) {
    val isVideo: Boolean get() = width > 0 && height > 0
    val totalBitrate: Int get() = videoBitrate + audioBitrate

    val formattedDuration: String
        get() {
            val totalSeconds = (durationMs / 1000).coerceAtLeast(0)
            val hours = totalSeconds / 3600
            val minutes = (totalSeconds % 3600) / 60
            val seconds = totalSeconds % 60
            return if (hours > 0) {
                String.format(Locale.US, "%d:%02d:%02d", hours, minutes, seconds)
            } else {
                String.format(Locale.US, "%02d:%02d", minutes, seconds)
            }
        }

    val formattedSize: String
        get() {
            if (sizeBytes <= 0) return "Desconocido"
            val kb = sizeBytes / 1024.0
            val mb = kb / 1024.0
            val gb = mb / 1024.0
            return when {
                gb >= 1.0 -> String.format(Locale.US, "%.2f GB", gb)
                mb >= 1.0 -> String.format(Locale.US, "%.1f MB", mb)
                else -> String.format(Locale.US, "%.0f KB", kb)
            }
        }

    val videoCodecSummary: String
        get() {
            val mime = videoMime ?: return "Ninguno"
            return when {
                mime.contains("avc", ignoreCase = true) || mime.contains("h264", ignoreCase = true) -> "H.264 / AVC"
                mime.contains("hevc", ignoreCase = true) || mime.contains("h265", ignoreCase = true) -> "H.265 / HEVC"
                mime.contains("vp9", ignoreCase = true) -> "VP9"
                mime.contains("vp8", ignoreCase = true) -> "VP8"
                mime.contains("av01", ignoreCase = true) || mime.contains("av1", ignoreCase = true) -> "AV1"
                mime.contains("mp4v", ignoreCase = true) -> "MPEG-4"
                else -> mime.substringAfterLast("/")
            }
        }

    val audioCodecSummary: String
        get() {
            val mime = audioMime ?: return "Ninguno"
            return when {
                mime.contains("mp4a-latm", ignoreCase = true) || mime.contains("aac", ignoreCase = true) -> "AAC"
                mime.contains("opus", ignoreCase = true) -> "Opus"
                mime.contains("mpeg", ignoreCase = true) || mime.contains("mp3", ignoreCase = true) -> "MP3"
                mime.contains("vorbis", ignoreCase = true) -> "Vorbis"
                mime.contains("flac", ignoreCase = true) -> "FLAC"
                else -> mime.substringAfterLast("/")
            }
        }
}

/**
 * Compression profiles matching Alenia Fuse PC strategies.
 */
enum class FuseCompressProfile(val title: String, val description: String) {
    BALANCED("Equilibrada", "Reduce el tamaño preservando excelente fidelidad visual (Recomendado)."),
    FAST("Rápida", "Compresión acelerada por hardware manteniendo resolución nativa."),
    HIGH("Máxima", "Compresión intensa (escala a 720p/bitrate ajustado) para menor peso."),
    TARGET_SIZE("Tamaño objetivo", "Calcula el bitrate óptimo exacto para ajustarse a los MB indicados.")
}

/**
 * Target export format.
 */
enum class FuseFormat(val extension: String, val mimeType: String, val isAudioOnly: Boolean) {
    MP4("mp4", "video/mp4", false),
    WEBM("webm", "video/webm", false),
    M4A("m4a", "audio/mp4", true),
    AAC("aac", "audio/aac", true)
}

/**
 * Complete plan for a Fuse transformation or compression operation.
 */
data class FusePlan(
    val inputUri: Uri? = null,
    val outputFileName: String,
    val targetFormat: FuseFormat,
    val videoBitrateBps: Int? = null,
    val audioBitrateBps: Int? = null,
    val targetWidth: Int? = null,
    val targetHeight: Int? = null,
    val trimStartMs: Long = 0L,
    val trimEndMs: Long = 0L,
    val remuxOnly: Boolean = false,
    val estimatedSizeMb: Float = 0f
)
