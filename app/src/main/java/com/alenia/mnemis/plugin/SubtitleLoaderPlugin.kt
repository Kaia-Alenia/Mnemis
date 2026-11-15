package com.alenia.mnemis.plugin

import android.content.Context
import android.net.Uri
import android.provider.OpenableColumns
import androidx.activity.ComponentActivity
import androidx.activity.result.ActivityResultLauncher
import androidx.activity.result.contract.ActivityResultContracts
import androidx.media3.common.C
import androidx.media3.common.MediaItem
import androidx.media3.common.MimeTypes
import androidx.media3.common.Player

/**
 * Subtitle loader plugin: lets the user pick an external SRT/VTT subtitle
 * file from device storage and attach it to the current Media3 playback.
 *
 * The file picking, subtitle parsing and the MediaItem track model live here
 * so PlayerActivity only needs one call (see [applyTo]). No playback
 * behaviour is changed until [applyTo] is called with a loaded track.
 */
object SubtitleLoaderPlugin : MnemisPlugin {

    override val id = "subtitles"
    override val name = "Subtitles"
    override val version = "v1.0.0"
    override val description = "Optional module to load external SRT/VTT subtitle tracks."
    override val isBuiltIn = true

    /** MIME type passed to the system file picker. "*\/*" is used because many
     * file managers do not map .srt/.vtt to a specific MIME type; the picked
     * file is validated by extension afterwards in [loadTrack]. */
    const val PICKER_MIME_TYPE = "*/*"

    /** Largest subtitle file accepted, to avoid loading a huge file into memory. */
    const val MAX_SUBTITLE_FILE_BYTES = 2 * 1024 * 1024

    /** Largest number of cues kept from one file. */
    const val MAX_CUES = 5_000

    enum class SubtitleFormat(val mimeType: String) {
        SRT(MimeTypes.APPLICATION_SUBRIP),
        VTT(MimeTypes.TEXT_VTT)
    }

    data class SubtitleCue(
        val startMs: Long,
        val endMs: Long,
        val text: String
    )

    data class SubtitleTrack(
        val uri: Uri,
        val label: String,
        val format: SubtitleFormat,
        val language: String = "und",
        val cues: List<SubtitleCue> = emptyList()
    ) {
        val cueCount: Int get() = cues.size

        fun toSubtitleConfiguration(): MediaItem.SubtitleConfiguration {
            return MediaItem.SubtitleConfiguration.Builder(uri)
                .setMimeType(format.mimeType)
                .setLanguage(language)
                .setSelectionFlags(C.SELECTION_FLAG_DEFAULT)
                .build()
        }
    }

    /** The track currently applied to playback, if any. */
    var activeTrack: SubtitleTrack? = null
        private set

    fun isSupportedFile(fileName: String?): Boolean =
        detectFormat(fileName) != null

    fun detectFormat(fileName: String?): SubtitleFormat? {
        if (fileName.isNullOrBlank()) return null
        val lower = fileName.lowercase()
        return when {
            lower.endsWith(".srt") -> SubtitleFormat.SRT
            lower.endsWith(".vtt") -> SubtitleFormat.VTT
            else -> null
        }
    }

    /**
     * Creates the system file picker launcher. Call this from the host
     * Activity's onCreate (registerForActivityResult must run before the
     * Activity is started). The callback receives the parsed track, or null
     * when the user cancels or the file is not a readable SRT/VTT file.
     */
    fun createPickerLauncher(
        activity: ComponentActivity,
        onTrackSelected: (SubtitleTrack?) -> Unit
    ): ActivityResultLauncher<String> {
        return activity.registerForActivityResult(ActivityResultContracts.GetContent()) { uri ->
            if (uri == null) {
                onTrackSelected(null)
                return@registerForActivityResult
            }
            onTrackSelected(loadTrack(activity, uri))
        }
    }

    /**
     * Reads and validates a subtitle file behind [uri]. Returns null when the
     * file has an unsupported extension, cannot be read, is too large, or
     * contains no parseable cues.
     */
    fun loadTrack(context: Context, uri: Uri): SubtitleTrack? {
        val fileName = resolveDisplayName(context, uri) ?: uri.lastPathSegment
        val format = detectFormat(fileName) ?: return null
        val text = runCatching {
            context.contentResolver.openInputStream(uri)?.use { input ->
                // Chunked read (instead of InputStream.readNBytes, which needs
                // API 33+) so the size cap also works on the minSdk 24 devices.
                val out = java.io.ByteArrayOutputStream()
                val buffer = ByteArray(8192)
                var total = 0
                var read: Int
                while (input.read(buffer).also { read = it } != -1) {
                    total += read
                    if (total > MAX_SUBTITLE_FILE_BYTES) return null
                    out.write(buffer, 0, read)
                }
                String(out.toByteArray(), Charsets.UTF_8)
            }
        }.getOrNull() ?: return null
        val cues = parseCues(text)
        if (cues.isEmpty()) return null
        val label = fileName?.substringBeforeLast('.')?.takeIf { it.isNotBlank() } ?: "Subtitles"
        return SubtitleTrack(uri = uri, label = label, format = format, cues = cues)
    }

    private fun resolveDisplayName(context: Context, uri: Uri): String? {
        return runCatching {
            context.contentResolver.query(
                uri,
                arrayOf(OpenableColumns.DISPLAY_NAME),
                null,
                null,
                null
            )?.use { cursor ->
                if (cursor.moveToFirst()) {
                    val index = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    if (index >= 0) cursor.getString(index) else null
                } else {
                    null
                }
            }
        }.getOrNull()
    }

    /**
     * Parses SRT and VTT text into cues. Both formats share the same cue
     * shape (an optional id line, a "start --> end" timestamp line, then text
     * lines), so one parser handles both. VTT headers, NOTE/STYLE blocks and
     * inline HTML tags are ignored.
     */
    fun parseCues(text: String): List<SubtitleCue> {
        val normalized = text.replace("\r\n", "\n").replace('\r', '\n')
        val cues = mutableListOf<SubtitleCue>()
        val blocks = normalized.split(Regex("\n\\s*\n"))
        for (block in blocks) {
            if (cues.size >= MAX_CUES) break
            val lines = block.lines().map { it.trim() }.filter { it.isNotEmpty() }
            if (lines.isEmpty()) continue
            val first = lines.first()
            if (first.startsWith("WEBVTT") || first.startsWith("NOTE") ||
                first.startsWith("STYLE") || first.startsWith("REGION")
            ) {
                continue
            }
            val stampIndex = lines.indexOfFirst { it.contains("-->") }
            if (stampIndex < 0) continue
            val stampLine = lines[stampIndex]
            val parts = stampLine.split("-->")
            if (parts.size != 2) continue
            val startMs = parseTimestamp(parts[0].trim().substringBefore(' ')) ?: continue
            val endMs = parseTimestamp(parts[1].trim().substringBefore(' ')) ?: continue
            if (endMs <= startMs) continue
            val cueText = lines.drop(stampIndex + 1)
                .joinToString("\n") { stripTags(it) }
                .trim()
            if (cueText.isEmpty()) continue
            cues.add(SubtitleCue(startMs = startMs, endMs = endMs, text = cueText))
        }
        return cues
    }

    private fun stripTags(line: String): String =
        line.replace(Regex("<[^>]*>"), "").trim()

    /**
     * Parses "HH:MM:SS,mmm", "HH:MM:SS.mmm" or "MM:SS.mmm" into milliseconds.
     * Returns null for anything else.
     */
    fun parseTimestamp(raw: String): Long? {
        val full = Regex("""(\d{1,2}):(\d{2}):(\d{2})[,.](\d{1,3})""").matchEntire(raw)
        if (full != null) {
            val (hours, minutes, seconds, millis) = full.destructured
            return hours.toLong() * 3_600_000L +
                minutes.toLong() * 60_000L +
                seconds.toLong() * 1_000L +
                millis.padEnd(3, '0').toLong()
        }
        val short = Regex("""(\d{1,2}):(\d{2})[,.](\d{1,3})""").matchEntire(raw)
        if (short != null) {
            val (minutes, seconds, millis) = short.destructured
            return minutes.toLong() * 60_000L +
                seconds.toLong() * 1_000L +
                millis.padEnd(3, '0').toLong()
        }
        return null
    }

    /**
     * Returns a copy of [base] with [track] added as its subtitle
     * configuration. The original item (uri, mime type, metadata) is kept.
     */
    fun getMediaItemWithSubtitles(base: MediaItem, track: SubtitleTrack): MediaItem {
        return base.buildUpon()
            .setSubtitleConfigurations(listOf(track.toSubtitleConfiguration()))
            .build()
    }

    /**
     * Attaches [track] to whatever the player is currently playing, keeping
     * the current position and play/pause state. Returns false when nothing
     * is loaded in the player.
     *
     * TODO(PlayerActivity hook): call this once from PlayerActivity when the
     * user picks a subtitle file, e.g. in the picker callback:
     *   SubtitleLoaderPlugin.applyTo(playerManager.exoPlayer, track)
     * (one line; no other PlayerActivity change is needed).
     */
    fun applyTo(player: Player?, track: SubtitleTrack): Boolean {
        val current = player?.currentMediaItem ?: return false
        val positionMs = player.currentPosition
        val wasPlaying = player.playWhenReady
        player.setMediaItem(getMediaItemWithSubtitles(current, track), positionMs)
        player.prepare()
        player.playWhenReady = wasPlaying
        activeTrack = track
        return true
    }

    /** Removes any subtitle track applied with [applyTo], keeping position and state. */
    fun clearSubtitles(player: Player?): Boolean {
        val current = player?.currentMediaItem ?: return false
        val positionMs = player.currentPosition
        val wasPlaying = player.playWhenReady
        val cleared = current.buildUpon()
            .setSubtitleConfigurations(emptyList())
            .build()
        player.setMediaItem(cleared, positionMs)
        player.prepare()
        player.playWhenReady = wasPlaying
        activeTrack = null
        return true
    }
}
