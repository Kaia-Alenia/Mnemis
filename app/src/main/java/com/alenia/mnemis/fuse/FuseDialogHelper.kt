package com.alenia.mnemis.fuse

import android.content.Intent
import android.media.MediaScannerConnection
import android.net.Uri
import android.view.LayoutInflater
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.alenia.mnemis.databinding.DialogFuseBinding
import com.alenia.mnemis.ui.PlayerActivity
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import java.io.File
import java.util.Locale

/**
 * UI dialog helper for Alenia Fuse operations in Mnemis.
 * Provides on-device technical inspection, intelligent compression profile selection,
 * format conversion, and real-time progress.
 */
object FuseDialogHelper {

    fun show(
        activity: AppCompatActivity,
        scope: CoroutineScope,
        videoUri: Uri,
        videoTitle: String,
        onMediaProcessed: ((File) -> Unit)? = null
    ) {
        val binding = DialogFuseBinding.inflate(LayoutInflater.from(activity))
        binding.tvFuseFileName.text = videoTitle

        val dialog = AlertDialog.Builder(activity)
            .setView(binding.root)
            .setCancelable(false)
            .create()

        var currentInfo: FuseMediaInfo? = null
        var activeJob: Job? = null

        // 1. Inspect media in background
        scope.launch(Dispatchers.IO) {
            val info = FusePlugin.inspect(activity, videoUri, videoTitle)
            withContext(Dispatchers.Main) {
                currentInfo = info
                val details = buildString {
                    append("• Contenedor/Códecs: ${info.videoCodecSummary} / ${info.audioCodecSummary}\n")
                    if (info.width > 0 && info.height > 0) {
                        val fps = if (info.frameRate > 0) " @ ${info.frameRate.toInt()} fps" else ""
                        append("• Resolución: ${info.width}x${info.height}$fps\n")
                    }
                    append("• Duración: ${info.formattedDuration}\n")
                    append("• Tamaño en disco: ${info.formattedSize}\n")
                    if (info.totalBitrate > 0) {
                        append("• Bitrate total: ${info.totalBitrate / 1000} kbps")
                    } else {
                        append("• Bitrate: Variable")
                    }
                }
                binding.tvFuseInfoDetails.text = details

                // Adjust default remux checkbox based on codec compatibility
                val isCopyable = FusePlanner.isStreamCopyCompatible(info.videoMime, info.audioMime, FuseFormat.MP4)
                binding.cbRemuxOnly.isEnabled = isCopyable
                if (!isCopyable) {
                    binding.cbRemuxOnly.isChecked = false
                    binding.cbRemuxOnly.text = "Remux rápido (No compatible con los códecs de este archivo)"
                }
            }
        }

        // Toggle action tabs
        binding.rgFuseAction.setOnCheckedChangeListener { _, checkedId ->
            if (checkedId == binding.rbActionCompress.id) {
                binding.layoutCompressOptions.visibility = View.VISIBLE
                binding.layoutConvertOptions.visibility = View.GONE
            } else {
                binding.layoutCompressOptions.visibility = View.GONE
                binding.layoutConvertOptions.visibility = View.VISIBLE
            }
        }

        // Toggle target size input
        binding.rgCompressProfile.setOnCheckedChangeListener { _, checkedId ->
            if (checkedId == binding.rbProfileTargetSize.id) {
                binding.tilTargetSizeMb.visibility = View.VISIBLE
            } else {
                binding.tilTargetSizeMb.visibility = View.GONE
            }
        }

        binding.btnCancelFuse.setOnClickListener {
            activeJob?.cancel()
            dialog.dismiss()
        }

        binding.btnStartFuse.setOnClickListener {
            val info = currentInfo ?: return@setOnClickListener

            // Determine plan
            val plan = if (binding.rbActionCompress.isChecked) {
                val profile = when (binding.rgCompressProfile.checkedRadioButtonId) {
                    binding.rbProfileFast.id -> FuseCompressProfile.FAST
                    binding.rbProfileHigh.id -> FuseCompressProfile.HIGH
                    binding.rbProfileTargetSize.id -> FuseCompressProfile.TARGET_SIZE
                    else -> FuseCompressProfile.BALANCED
                }
                val targetSizeMb = binding.etTargetSizeMb.text?.toString()?.toFloatOrNull() ?: 15.0f
                FusePlugin.planCompression(info, profile, targetSizeMb)
            } else {
                val format = when (binding.rgConvertFormat.checkedRadioButtonId) {
                    binding.rbFormatWebm.id -> FuseFormat.WEBM
                    binding.rbFormatAudioAac.id -> FuseFormat.M4A
                    else -> FuseFormat.MP4
                }
                val allowRemux = binding.cbRemuxOnly.isChecked
                FusePlugin.planConversion(info, format, allowRemux)
            }

            // Lock UI and show progress
            binding.btnStartFuse.isEnabled = false
            binding.btnCancelFuse.text = "Cancelar operación"
            binding.rgFuseAction.isEnabled = false
            binding.layoutFuseProgress.visibility = View.VISIBLE
            binding.progressBarFuse.progress = 0
            binding.tvFusePercent.text = "0%"
            binding.tvFuseStatus.text = "Procesando: ${plan.outputFileName}…"

            activeJob = FusePlugin.execute(
                context = activity,
                scope = scope,
                plan = plan,
                listener = object : FuseTranscoderEngine.ProgressListener {
                    override fun onProgress(percentage: Int) {
                        binding.progressBarFuse.progress = percentage
                        binding.tvFusePercent.text = "$percentage%"
                    }

                    override fun onSuccess(outputFile: File, elapsedSeconds: Double) {
                        // Rescan file so it appears in MediaStore/Mnemis video list
                        MediaScannerConnection.scanFile(
                            activity,
                            arrayOf(outputFile.absolutePath),
                            null,
                            null
                        )

                        val msg = String.format(
                            Locale.US,
                            "Alenia Fuse completó la tarea en %.1fs:\n%s",
                            elapsedSeconds,
                            outputFile.name
                        )
                        Toast.makeText(activity, msg, Toast.LENGTH_LONG).show()

                        dialog.dismiss()
                        onMediaProcessed?.invoke(outputFile)

                        // Ask if user wants to play now
                        AlertDialog.Builder(activity)
                            .setTitle("◈ Alenia Fuse — Operación Exitosa")
                            .setMessage("El archivo fue generado exitosamente:\n${outputFile.name}\n\n¿Deseas reproducirlo ahora?")
                            .setPositiveButton("Reproducir") { _, _ ->
                                val playIntent = Intent(activity, PlayerActivity::class.java).apply {
                                    putExtra(PlayerActivity.EXTRA_VIDEO_URI, Uri.fromFile(outputFile).toString())
                                    putExtra(PlayerActivity.EXTRA_VIDEO_TITLE, outputFile.name)
                                }
                                activity.startActivity(playIntent)
                            }
                            .setNegativeButton("Listo", null)
                            .show()
                    }

                    override fun onError(error: String) {
                        binding.tvFuseStatus.text = "Error: $error"
                        binding.btnStartFuse.isEnabled = true
                        binding.btnCancelFuse.text = "Cerrar"
                        Toast.makeText(activity, "Fuse error: $error", Toast.LENGTH_LONG).show()
                    }
                }
            )
        }

        dialog.show()
    }
}
