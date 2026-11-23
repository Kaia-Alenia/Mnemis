package com.alenia.mnemis.fuse

import android.content.Context
import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import android.media.MediaMuxer
import android.net.Uri
import android.os.Environment
import androidx.media3.common.MediaItem
import androidx.media3.common.MimeTypes
import androidx.media3.transformer.Composition
import androidx.media3.transformer.EditedMediaItem
import androidx.media3.transformer.ExportException
import androidx.media3.transformer.ExportResult
import androidx.media3.transformer.ProgressHolder
import androidx.media3.transformer.Transformer
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.nio.ByteBuffer

/**
 * Execution engine for Alenia Fuse operations on Android.
 * Handles fast container remuxing via MediaMuxer, as well as hardware-accelerated
 * compression and transcoding via AndroidX Media3 Transformer.
 */
object FuseTranscoderEngine {

    interface ProgressListener {
        fun onProgress(percentage: Int)
        fun onSuccess(outputFile: File, elapsedSeconds: Double)
        fun onError(error: String)
    }

    /**
     * Resolves or creates the output directory in public Movies/Mnemis_Fuse or app external files.
     */
    fun getOutputDirectory(context: Context): File {
        val moviesDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_MOVIES)
        val target = if (moviesDir != null && moviesDir.canWrite()) {
            File(moviesDir, "Mnemis_Fuse")
        } else {
            File(context.getExternalFilesDir(Environment.DIRECTORY_MOVIES), "Mnemis_Fuse")
        }
        if (!target.exists()) {
            target.mkdirs()
        }
        return target
    }

    /**
     * Executes the given FusePlan asynchronously.
     */
    fun execute(
        context: Context,
        scope: CoroutineScope,
        plan: FusePlan,
        listener: ProgressListener
    ): Job {
        return scope.launch(Dispatchers.Default) {
            val startTimeMs = System.currentTimeMillis()
            val outDir = getOutputDirectory(context)
            var safeOutName = plan.outputFileName.replace(Regex("[^a-zA-Z0-9._-]"), "_")
            var outFile = File(outDir, safeOutName)
            var counter = 1
            while (outFile.exists()) {
                val base = safeOutName.substringBeforeLast('.')
                val ext = safeOutName.substringAfterLast('.', "mp4")
                outFile = File(outDir, "${base}_$counter.$ext")
                counter++
            }

            try {
                val inputUri = plan.inputUri ?: error("URI de entrada no válida para procesar con Fuse")
                if (plan.remuxOnly) {
                    // Fast remux: stream copy without re-encoding
                    remuxDirectly(context, inputUri, outFile, listener)
                } else {
                    // Hardware transcode / compress using AndroidX Media3 Transformer
                    transcodeWithTransformer(context, plan, inputUri, outFile, listener)
                }

                val elapsedSec = (System.currentTimeMillis() - startTimeMs) / 1000.0
                withContext(Dispatchers.Main) {
                    listener.onSuccess(outFile, elapsedSec)
                }
            } catch (e: CancellationException) {
                if (outFile.exists()) outFile.delete()
                withContext(Dispatchers.Main) {
                    listener.onError("Operación cancelada")
                }
            } catch (e: Exception) {
                if (outFile.exists()) outFile.delete()
                withContext(Dispatchers.Main) {
                    listener.onError(e.message ?: "Error desconocido durante el procesamiento de Fuse")
                }
            }
        }
    }

    /**
     * Fast Remux: copies video/audio tracks directly using MediaExtractor and MediaMuxer.
     * Near instant speed, zero quality degradation.
     */
    private suspend fun remuxDirectly(
        context: Context,
        inputUri: Uri,
        outputFile: File,
        listener: ProgressListener
    ) = withContext(Dispatchers.IO) {
        val extractor = MediaExtractor()
        var muxer: MediaMuxer? = null
        try {
            extractor.setDataSource(context, inputUri, null)
            val trackCount = extractor.trackCount
            val outFormat = if (outputFile.extension.equals("webm", ignoreCase = true)) {
                MediaMuxer.OutputFormat.MUXER_OUTPUT_WEBM
            } else {
                MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4
            }

            muxer = MediaMuxer(outputFile.absolutePath, outFormat)
            val trackMap = mutableMapOf<Int, Int>()

            for (i in 0 until trackCount) {
                val format = extractor.getTrackFormat(i)
                val mime = format.getString(MediaFormat.KEY_MIME) ?: continue
                if (mime.startsWith("video/") || mime.startsWith("audio/")) {
                    extractor.selectTrack(i)
                    val muxerTrack = muxer.addTrack(format)
                    trackMap[i] = muxerTrack
                }
            }

            muxer.start()

            val bufferSize = 1024 * 1024 // 1MB buffer
            val buffer = ByteBuffer.allocateDirect(bufferSize)
            val bufferInfo = MediaCodec.BufferInfo()

            var lastReportTime = 0L
            while (isActive) {
                bufferInfo.offset = 0
                val sampleSize = extractor.readSampleData(buffer, 0)
                if (sampleSize < 0) {
                    break
                }
                bufferInfo.size = sampleSize
                bufferInfo.presentationTimeUs = extractor.sampleTime
                bufferInfo.flags = extractor.sampleFlags

                val trackIndex = extractor.sampleTrackIndex
                val muxerTrackIndex = trackMap[trackIndex]
                if (muxerTrackIndex != null) {
                    muxer.writeSampleData(muxerTrackIndex, buffer, bufferInfo)
                }

                extractor.advance()

                val now = System.currentTimeMillis()
                if (now - lastReportTime > 250) {
                    lastReportTime = now
                    // Estimated progress based on presentation time vs duration
                    withContext(Dispatchers.Main) {
                        listener.onProgress(50)
                    }
                }
            }

            withContext(Dispatchers.Main) {
                listener.onProgress(100)
            }
        } finally {
            runCatching { extractor.release() }
            runCatching {
                muxer?.stop()
                muxer?.release()
            }
        }
    }

    /**
     * Transcoding and compression using Media3 Transformer.
     */
    private suspend fun transcodeWithTransformer(
        context: Context,
        plan: FusePlan,
        inputUri: Uri,
        outputFile: File,
        listener: ProgressListener
    ) = withContext(Dispatchers.Main) {
        val transformerBuilder = Transformer.Builder(context)

        // Set target container and audio/video MIME types
        val targetVideoMime = when (plan.targetFormat) {
            FuseFormat.WEBM -> MimeTypes.VIDEO_WEBM
            else -> MimeTypes.VIDEO_H264
        }
        val targetAudioMime = when (plan.targetFormat) {
            FuseFormat.WEBM -> MimeTypes.AUDIO_OPUS
            FuseFormat.MP4 -> MimeTypes.AUDIO_AAC
            FuseFormat.M4A, FuseFormat.AAC -> MimeTypes.AUDIO_AAC
        }

        if (plan.targetFormat.isAudioOnly) {
            transformerBuilder.setAudioMimeType(targetAudioMime)
        } else {
            transformerBuilder.setVideoMimeType(targetVideoMime)
            transformerBuilder.setAudioMimeType(targetAudioMime)
        }

        var completionError: Throwable? = null
        var isCompleted = false

        transformerBuilder.addListener(object : Transformer.Listener {
            override fun onCompleted(composition: Composition, exportResult: ExportResult) {
                isCompleted = true
            }

            override fun onError(
                composition: Composition,
                exportResult: ExportResult,
                exportException: ExportException
            ) {
                completionError = exportException
                isCompleted = true
            }
        })

        val transformer = transformerBuilder.build()

        val mediaItemBuilder = MediaItem.Builder().setUri(inputUri)
        if (plan.trimStartMs > 0 || plan.trimEndMs > plan.trimStartMs) {
            val clippingConfig = MediaItem.ClippingConfiguration.Builder()
                .setStartPositionMs(plan.trimStartMs)
            if (plan.trimEndMs > plan.trimStartMs) {
                clippingConfig.setEndPositionMs(plan.trimEndMs)
            }
            mediaItemBuilder.setClippingConfiguration(clippingConfig.build())
        }

        val editedMediaItem = EditedMediaItem.Builder(mediaItemBuilder.build())
            .setRemoveVideo(plan.targetFormat.isAudioOnly)
            .build()

        transformer.start(editedMediaItem, outputFile.absolutePath)

        val progressHolder = ProgressHolder()
        while (!isCompleted && isActive) {
            delay(200)
            val state = transformer.getProgress(progressHolder)
            if (state == Transformer.PROGRESS_STATE_AVAILABLE) {
                listener.onProgress(progressHolder.progress)
            }
        }

        if (!isActive && !isCompleted) {
            transformer.cancel()
            throw CancellationException("Cancelado por el usuario")
        }

        completionError?.let {
            throw it
        }

        listener.onProgress(100)
    }
}
