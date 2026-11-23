package com.alenia.mnemis.fuse

import java.io.File
import kotlin.math.roundToInt

/**
 * Operation planner adapted from Alenia Fuse PC (OperationPlanner).
 * Decides whether to remux (stream copy) or transcode, calculates target bitrates,
 * resolutions, and output filenames.
 */
object FusePlanner {

    private const val MIN_VIDEO_BITRATE = 180_000 // 180 kbps floor
    private const val MAX_VIDEO_BITRATE = 20_000_000 // 20 Mbps ceiling

    /**
     * Checks if source codecs are directly compatible with the target container
     * without needing to re-encode (Stream Copy / Remux).
     */
    fun isStreamCopyCompatible(
        sourceVideoMime: String?,
        sourceAudioMime: String?,
        targetFormat: FuseFormat
    ): Boolean {
        if (targetFormat.isAudioOnly) {
            val aMime = sourceAudioMime?.lowercase().orEmpty()
            return when (targetFormat) {
                FuseFormat.M4A, FuseFormat.AAC -> aMime.contains("mp4a") || aMime.contains("aac")
                else -> false
            }
        }

        val vMime = sourceVideoMime?.lowercase().orEmpty()
        val aMime = sourceAudioMime?.lowercase().orEmpty()

        val isVideoCopyable = when (targetFormat) {
            FuseFormat.MP4 -> vMime.contains("avc") || vMime.contains("h264") || vMime.contains("hevc") || vMime.contains("h265")
            FuseFormat.WEBM -> vMime.contains("vp8") || vMime.contains("vp9") || vMime.contains("av01") || vMime.contains("av1")
            else -> false
        }

        val isAudioCopyable = when (targetFormat) {
            FuseFormat.MP4 -> aMime.isEmpty() || aMime.contains("mp4a") || aMime.contains("aac") || aMime.contains("mp3")
            FuseFormat.WEBM -> aMime.isEmpty() || aMime.contains("opus") || aMime.contains("vorbis")
            else -> false
        }

        return isVideoCopyable && isAudioCopyable
    }

    /**
     * Computes optimal video bitrate in bps for a specified target size in Megabytes.
     */
    fun calculateBitrateForSize(
        targetSizeMb: Float,
        durationMs: Long,
        audioBitrateBps: Int = 128_000
    ): Int {
        if (targetSizeMb <= 0f || durationMs <= 0L) {
            return 1_200_000 // Fallback to 1.2 Mbps
        }
        val durationSec = durationMs / 1000.0
        val totalBits = targetSizeMb * 8.0 * 1024.0 * 1024.0
        val totalBitrateBps = totalBits / durationSec
        val targetVideoBitrate = (totalBitrateBps - audioBitrateBps).roundToInt()
        return targetVideoBitrate.coerceIn(MIN_VIDEO_BITRATE, MAX_VIDEO_BITRATE)
    }

    /**
     * Creates a compression plan based on the chosen profile.
     */
    fun planCompression(
        info: FuseMediaInfo,
        profile: FuseCompressProfile,
        targetSizeMb: Float? = null,
        trimStartMs: Long = 0L,
        trimEndMs: Long = 0L
    ): FusePlan {
        val effectiveDuration = if (trimEndMs > trimStartMs) {
            trimEndMs - trimStartMs
        } else {
            info.durationMs.coerceAtLeast(1000L)
        }

        val originalBitrate = if (info.videoBitrate > 0) {
            info.videoBitrate
        } else if (info.sizeBytes > 0 && info.durationMs > 0) {
            ((info.sizeBytes * 8.0) / (info.durationMs / 1000.0)).toInt() - 128_000
        } else {
            2_500_000
        }.coerceIn(MIN_VIDEO_BITRATE, MAX_VIDEO_BITRATE)

        var targetVideoBitrate: Int
        var targetAudioBitrate = 128_000
        var targetWidth: Int? = null
        var targetHeight: Int? = null

        when (profile) {
            FuseCompressProfile.BALANCED -> {
                // Fuse balanced: ~55-60% of original bitrate, keep native resolution if <= 1080p
                targetVideoBitrate = (originalBitrate * 0.60).roundToInt().coerceIn(600_000, 2_200_000)
                targetAudioBitrate = 128_000
                if (info.height > 1080) {
                    targetHeight = 1080
                    targetWidth = ((1080.0 / info.height) * info.width).roundToInt()
                }
            }
            FuseCompressProfile.FAST -> {
                // Fuse fast: moderate compression, fast hardware preset, keep resolution
                targetVideoBitrate = (originalBitrate * 0.75).roundToInt().coerceIn(900_000, 3_500_000)
                targetAudioBitrate = 128_000
            }
            FuseCompressProfile.HIGH -> {
                // Fuse high: aggressive size reduction (~35-40%), scale down to 720p if higher
                targetVideoBitrate = (originalBitrate * 0.38).roundToInt().coerceIn(350_000, 1_100_000)
                targetAudioBitrate = 96_000
                if (info.height > 720) {
                    targetHeight = 720
                    targetWidth = ((720.0 / info.height) * info.width).roundToInt()
                }
            }
            FuseCompressProfile.TARGET_SIZE -> {
                val size = targetSizeMb ?: 15.0f // Default 15 MB for Discord/Telegram sharing
                targetAudioBitrate = if (size < 10f) 64_000 else 128_000
                targetVideoBitrate = calculateBitrateForSize(size, effectiveDuration, targetAudioBitrate)
                if (targetVideoBitrate < 600_000 && info.height > 720) {
                    targetHeight = 720
                    targetWidth = ((720.0 / info.height) * info.width).roundToInt()
                }
            }
        }

        // Even dimensions required for H.264 encoders
        if (targetWidth != null && targetWidth % 2 != 0) targetWidth -= 1
        if (targetHeight != null && targetHeight % 2 != 0) targetHeight -= 1

        val baseName = info.name.substringBeforeLast('.')
        val outName = "${baseName}_fuse_compressed.mp4"
        val estimatedTotalBitrate = targetVideoBitrate + targetAudioBitrate
        val estimatedMb = ((estimatedTotalBitrate.toDouble() * (effectiveDuration / 1000.0)) / (8.0 * 1024.0 * 1024.0)).toFloat()

        return FusePlan(
            inputUri = info.uri,
            outputFileName = outName,
            targetFormat = FuseFormat.MP4,
            videoBitrateBps = targetVideoBitrate,
            audioBitrateBps = targetAudioBitrate,
            targetWidth = targetWidth,
            targetHeight = targetHeight,
            trimStartMs = trimStartMs,
            trimEndMs = trimEndMs,
            remuxOnly = false,
            estimatedSizeMb = estimatedMb
        )
    }

    /**
     * Creates a format conversion or audio extraction plan.
     */
    fun planConversion(
        info: FuseMediaInfo,
        targetFormat: FuseFormat,
        allowRemux: Boolean = true,
        trimStartMs: Long = 0L,
        trimEndMs: Long = 0L
    ): FusePlan {
        val baseName = info.name.substringBeforeLast('.')
        val canRemux = allowRemux && trimStartMs == 0L && trimEndMs == 0L &&
            isStreamCopyCompatible(info.videoMime, info.audioMime, targetFormat)

        val outName = when {
            targetFormat.isAudioOnly -> "${baseName}_fuse_audio.${targetFormat.extension}"
            canRemux -> "${baseName}_fuse_remuxed.${targetFormat.extension}"
            else -> "${baseName}_fuse_converted.${targetFormat.extension}"
        }

        val effectiveDuration = if (trimEndMs > trimStartMs) {
            trimEndMs - trimStartMs
        } else {
            info.durationMs.coerceAtLeast(1000L)
        }

        val videoBitrate = if (targetFormat.isAudioOnly) {
            null
        } else {
            info.videoBitrate.takeIf { it > 0 } ?: 2_000_000
        }

        val audioBitrate = when (targetFormat) {
            FuseFormat.M4A, FuseFormat.AAC -> 192_000
            FuseFormat.WEBM -> 128_000
            FuseFormat.MP4 -> 160_000
        }

        val estimatedMb = if (canRemux) {
            (info.sizeBytes / (1024f * 1024f))
        } else {
            val totalBitrate = (videoBitrate ?: 0) + audioBitrate
            ((totalBitrate.toDouble() * (effectiveDuration / 1000.0)) / (8.0 * 1024.0 * 1024.0)).toFloat()
        }

        return FusePlan(
            inputUri = info.uri,
            outputFileName = outName,
            targetFormat = targetFormat,
            videoBitrateBps = videoBitrate,
            audioBitrateBps = audioBitrate,
            trimStartMs = trimStartMs,
            trimEndMs = trimEndMs,
            remuxOnly = canRemux,
            estimatedSizeMb = estimatedMb
        )
    }
}
