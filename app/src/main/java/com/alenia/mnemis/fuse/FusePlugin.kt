package com.alenia.mnemis.fuse

import android.content.Context
import android.net.Uri
import com.alenia.mnemis.plugin.MnemisPlugin
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job

/**
 * Official Alenia Fuse Plugin for Mnemis.
 * Unifies all Alenia Fuse PC abilities into a single native Android toolkit:
 * inspection, intelligent compression, container conversion, remuxing,
 * and audio extraction.
 */
object FusePlugin : MnemisPlugin {

    override val id: String = "alenia_fuse"
    override val name: String = "Alenia Fuse"
    override val version: String = "v2.0.7"
    override val description: String =
        "Suite multimedia profesional de Alenia Studios: inspección técnica profunda, compresión inteligente, conversión de formatos, remuxing y extracción de audio."
    override val isBuiltIn: Boolean = true

    /**
     * Inspects a media file on-device and returns detailed technical metadata.
     */
    fun inspect(context: Context, uri: Uri, fallbackTitle: String? = null): FuseMediaInfo {
        return FuseInspector.inspect(context, uri, fallbackTitle)
    }

    /**
     * Creates an intelligent compression plan based on Fuse PC presets.
     */
    fun planCompression(
        info: FuseMediaInfo,
        profile: FuseCompressProfile,
        targetSizeMb: Float? = null,
        trimStartMs: Long = 0L,
        trimEndMs: Long = 0L
    ): FusePlan {
        return FusePlanner.planCompression(info, profile, targetSizeMb, trimStartMs, trimEndMs)
    }

    /**
     * Creates a format conversion, remux, or audio extraction plan.
     */
    fun planConversion(
        info: FuseMediaInfo,
        targetFormat: FuseFormat,
        allowRemux: Boolean = true,
        trimStartMs: Long = 0L,
        trimEndMs: Long = 0L
    ): FusePlan {
        return FusePlanner.planConversion(info, targetFormat, allowRemux, trimStartMs, trimEndMs)
    }

    /**
     * Launches the execution of a Fuse operation in background.
     */
    fun execute(
        context: Context,
        scope: CoroutineScope,
        plan: FusePlan,
        listener: FuseTranscoderEngine.ProgressListener
    ): Job {
        return FuseTranscoderEngine.execute(context, scope, plan, listener)
    }
}
