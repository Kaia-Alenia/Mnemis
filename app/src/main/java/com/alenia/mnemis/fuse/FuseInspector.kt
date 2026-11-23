package com.alenia.mnemis.fuse

import android.content.Context
import android.media.MediaExtractor
import android.media.MediaFormat
import android.media.MediaMetadataRetriever
import android.net.Uri
import android.provider.OpenableColumns
import java.io.File

/**
 * Technical media inspector providing deep metadata inspection modeled after
 * Alenia Fuse CLI `/info` and `Media.inspect()`.
 */
object FuseInspector {

    fun inspect(context: Context, uri: Uri, fallbackTitle: String? = null): FuseMediaInfo {
        var name = fallbackTitle ?: "Video"
        var sizeBytes = 0L

        // Attempt resolving display name and size from content resolver
        if ("content".equals(uri.scheme, ignoreCase = true)) {
            runCatching {
                context.contentResolver.query(uri, null, null, null, null)?.use { cursor ->
                    val nameIdx = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    val sizeIdx = cursor.getColumnIndex(OpenableColumns.SIZE)
                    if (cursor.moveToFirst()) {
                        if (nameIdx >= 0) name = cursor.getString(nameIdx) ?: name
                        if (sizeIdx >= 0) sizeBytes = cursor.getLong(sizeIdx)
                    }
                }
            }
        } else if ("file".equals(uri.scheme, ignoreCase = true)) {
            val path = uri.path
            if (path != null) {
                val file = File(path)
                if (file.exists()) {
                    name = file.name
                    sizeBytes = file.length()
                }
            }
        }

        var durationMs = 0L
        var width = 0
        var height = 0
        var videoBitrate = 0
        var rotation = 0
        var frameRate = 0f

        val retriever = MediaMetadataRetriever()
        try {
            retriever.setDataSource(context, uri)
            durationMs = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_DURATION)?.toLongOrNull() ?: 0L
            width = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_WIDTH)?.toIntOrNull() ?: 0
            height = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_HEIGHT)?.toIntOrNull() ?: 0
            videoBitrate = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_BITRATE)?.toIntOrNull() ?: 0
            rotation = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION)?.toIntOrNull() ?: 0

            val captureFps = retriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_CAPTURE_FRAMERATE)?.toFloatOrNull()
            if (captureFps != null && captureFps > 0f) {
                frameRate = captureFps
            }
        } catch (_: Exception) {
            // Best effort metadata retrieval
        } finally {
            runCatching { retriever.release() }
        }

        var videoMime: String? = null
        var audioMime: String? = null
        var sampleRate = 0
        var channelCount = 0

        val extractor = MediaExtractor()
        try {
            extractor.setDataSource(context, uri, null)
            val trackCount = extractor.trackCount
            for (i in 0 until trackCount) {
                val format = extractor.getTrackFormat(i)
                val mime = format.getString(MediaFormat.KEY_MIME) ?: continue
                if (mime.startsWith("video/") && videoMime == null) {
                    videoMime = mime
                    if (width == 0 && format.containsKey(MediaFormat.KEY_WIDTH)) {
                        width = format.getInteger(MediaFormat.KEY_WIDTH)
                    }
                    if (height == 0 && format.containsKey(MediaFormat.KEY_HEIGHT)) {
                        height = format.getInteger(MediaFormat.KEY_HEIGHT)
                    }
                    if (frameRate <= 0f && format.containsKey(MediaFormat.KEY_FRAME_RATE)) {
                        frameRate = runCatching { format.getInteger(MediaFormat.KEY_FRAME_RATE).toFloat() }
                            .getOrElse { format.getFloat(MediaFormat.KEY_FRAME_RATE) }
                    }
                    if (videoBitrate == 0 && format.containsKey(MediaFormat.KEY_BIT_RATE)) {
                        videoBitrate = format.getInteger(MediaFormat.KEY_BIT_RATE)
                    }
                } else if (mime.startsWith("audio/") && audioMime == null) {
                    audioMime = mime
                    if (format.containsKey(MediaFormat.KEY_SAMPLE_RATE)) {
                        sampleRate = format.getInteger(MediaFormat.KEY_SAMPLE_RATE)
                    }
                    if (format.containsKey(MediaFormat.KEY_CHANNEL_COUNT)) {
                        channelCount = format.getInteger(MediaFormat.KEY_CHANNEL_COUNT)
                    }
                }
            }
        } catch (_: Exception) {
            // Non-fatal, keep data from retriever
        } finally {
            runCatching { extractor.release() }
        }

        return FuseMediaInfo(
            uri = uri,
            name = name,
            path = uri.path,
            durationMs = durationMs,
            sizeBytes = sizeBytes,
            width = width,
            height = height,
            videoMime = videoMime,
            audioMime = audioMime,
            videoBitrate = videoBitrate,
            frameRate = frameRate,
            channelCount = channelCount,
            sampleRate = sampleRate,
            rotation = rotation
        )
    }
}
