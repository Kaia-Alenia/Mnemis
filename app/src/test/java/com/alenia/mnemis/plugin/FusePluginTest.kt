package com.alenia.mnemis.plugin

import android.net.Uri
import com.alenia.mnemis.fuse.FuseCompressProfile
import com.alenia.mnemis.fuse.FuseFormat
import com.alenia.mnemis.fuse.FuseMediaInfo
import com.alenia.mnemis.fuse.FusePlanner
import com.alenia.mnemis.fuse.FusePlugin
import com.alenia.mnemis.ui.MainActivity
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * Unit tests for Alenia Fuse plugin, planner strategies, remux compatibility,
 * and streaming URL extraction.
 */
class FusePluginTest {

    @Test
    fun fusePlugin_metadataAndRegistry() {
        assertEquals("alenia_fuse", FusePlugin.id)
        assertEquals("Alenia Fuse", FusePlugin.name)
        assertEquals("v2.0.7", FusePlugin.version)
        assertTrue(FusePlugin.isBuiltIn)
        assertTrue(FusePlugin.description.contains("Alenia Studios"))

        val registered = MnemisPluginRegistry.allPlugins().find { it.id == FusePlugin.id }
        assertNotNull("Alenia Fuse must be registered in MnemisPluginRegistry", registered)
        assertEquals(FusePlugin.name, registered?.name)
    }

    @Test
    fun fusePlanner_streamCopyCompatibility() {
        // H.264 + AAC in MP4 -> compatible for remux without re-encoding
        assertTrue(
            FusePlanner.isStreamCopyCompatible(
                sourceVideoMime = "video/avc",
                sourceAudioMime = "audio/mp4a-latm",
                targetFormat = FuseFormat.MP4
            )
        )

        // HEVC + AAC in MP4 -> compatible
        assertTrue(
            FusePlanner.isStreamCopyCompatible(
                sourceVideoMime = "video/hevc",
                sourceAudioMime = "audio/mp4a-latm",
                targetFormat = FuseFormat.MP4
            )
        )

        // VP9 + Opus in MP4 -> not copyable, must transcode
        assertFalse(
            FusePlanner.isStreamCopyCompatible(
                sourceVideoMime = "video/x-vnd.on2.vp9",
                sourceAudioMime = "audio/opus",
                targetFormat = FuseFormat.MP4
            )
        )

        // VP9 + Opus in WebM -> compatible for remux
        assertTrue(
            FusePlanner.isStreamCopyCompatible(
                sourceVideoMime = "video/x-vnd.on2.vp9",
                sourceAudioMime = "audio/opus",
                targetFormat = FuseFormat.WEBM
            )
        )

        // Audio extraction of AAC into M4A/AAC -> copyable
        assertTrue(
            FusePlanner.isStreamCopyCompatible(
                sourceVideoMime = "video/avc",
                sourceAudioMime = "audio/mp4a-latm",
                targetFormat = FuseFormat.M4A
            )
        )
    }

    @Test
    fun fusePlanner_calculateBitrateForSize() {
        // 10 MB file for a 60-second video with 128 kbps audio
        // Total bits = 10 * 8 * 1024 * 1024 = 83,886,080 bits
        // Total bitrate = 83,886,080 / 60 = 1,398,101 bps
        // Video bitrate = 1,398,101 - 128,000 = 1,270,101 bps
        val bitrate = FusePlanner.calculateBitrateForSize(
            targetSizeMb = 10.0f,
            durationMs = 60_000L,
            audioBitrateBps = 128_000
        )
        assertTrue("Calculated video bitrate should be around 1.27 Mbps", bitrate in 1_250_000..1_290_000)
    }

    @Test
    fun fusePlanner_planCompressionProfiles() {
        val dummyUri: Uri? = null
        val info = FuseMediaInfo(
            uri = dummyUri,
            name = "gameplay.mp4",
            durationMs = 120_000L, // 2 minutes
            sizeBytes = 150 * 1024 * 1024L, // 150 MB
            width = 1920,
            height = 1080,
            videoMime = "video/avc",
            audioMime = "audio/mp4a-latm",
            videoBitrate = 10_000_000 // 10 Mbps
        )

        // 1. Balanced profile
        val balancedPlan = FusePlanner.planCompression(info, FuseCompressProfile.BALANCED)
        assertEquals(FuseFormat.MP4, balancedPlan.targetFormat)
        assertFalse(balancedPlan.remuxOnly)
        assertTrue(balancedPlan.outputFileName.endsWith("_fuse_compressed.mp4"))
        assertTrue("Video bitrate should be capped and reduced", balancedPlan.videoBitrateBps!! <= 2_500_000)
        assertTrue("Estimated size should be much smaller than 150MB", balancedPlan.estimatedSizeMb < 40f)

        // 2. High profile (should scale down to 720p)
        val highPlan = FusePlanner.planCompression(info, FuseCompressProfile.HIGH)
        assertEquals(720, highPlan.targetHeight)
        assertEquals(1280, highPlan.targetWidth)
        assertTrue(highPlan.videoBitrateBps!! <= 1_100_000)

        // 3. Target size profile (15 MB)
        val targetSizePlan = FusePlanner.planCompression(info, FuseCompressProfile.TARGET_SIZE, targetSizeMb = 15.0f)
        assertTrue("Estimated size should be close to 15 MB", targetSizePlan.estimatedSizeMb in 13.0f..17.0f)
    }

    @Test
    fun fusePlanner_planConversionAndRemux() {
        val dummyUri: Uri? = null
        val infoH264 = FuseMediaInfo(
            uri = dummyUri,
            name = "movie.mkv",
            durationMs = 60_000L,
            sizeBytes = 25 * 1024 * 1024L,
            width = 1920,
            height = 1080,
            videoMime = "video/avc",
            audioMime = "audio/mp4a-latm"
        )

        // MKV to MP4 with compatible H264+AAC codecs -> Remux without re-encoding!
        val remuxPlan = FusePlanner.planConversion(infoH264, FuseFormat.MP4, allowRemux = true)
        assertTrue("H264/AAC MKV to MP4 should be planned as direct remux", remuxPlan.remuxOnly)
        assertTrue(remuxPlan.outputFileName.contains("_fuse_remuxed.mp4"))

        // Audio extraction
        val audioPlan = FusePlanner.planConversion(infoH264, FuseFormat.M4A)
        assertTrue(audioPlan.targetFormat.isAudioOnly)
        assertTrue(audioPlan.outputFileName.contains("_fuse_audio.m4a"))
    }

    @Test
    fun mainActivity_extractUrlFromSharedText() {
        // YouTube share from app
        val ytShared = "Check this awesome video: https://youtu.be/dQw4w9WgXcQ watch now"
        assertEquals("https://youtu.be/dQw4w9WgXcQ", MainActivity.extractUrlFromSharedText(ytShared))

        // Twitch stream share
        val twitchShared = "Watch channel live on Twitch: https://m.twitch.tv/alenia_studios"
        assertEquals("https://m.twitch.tv/alenia_studios", MainActivity.extractUrlFromSharedText(twitchShared))

        // Direct HLS stream
        val hlsShared = "https://stream.server.org/live/master.m3u8"
        assertEquals("https://stream.server.org/live/master.m3u8", MainActivity.extractUrlFromSharedText(hlsShared))

        // Plain text without URL
        assertNull(MainActivity.extractUrlFromSharedText("Hola amigos sin enlace"))
        assertNull(MainActivity.extractUrlFromSharedText(null))
    }
}
