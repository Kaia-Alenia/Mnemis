package com.alenia.mnemis.plugin

import java.util.concurrent.CopyOnWriteArrayList

/**
 * Stream sniffer plugin: read-only detector for direct HLS (.m3u8) and MP4
 * stream URLs seen in WebView traffic.
 *
 * Detection runs through the hooks WebPlayerActivity already calls for every
 * matching web plugin: [shouldBlockRequest] sees each intercepted request
 * (including the actual fetches behind a page's <video> src), and
 * [getUserScript] injects a small page script that collects <video> /
 * <source> src attributes into window.__mnemisDetectedStreams for pages
 * whose streams have not started loading yet.
 *
 * This plugin never blocks anything: [shouldBlockRequest] always returns
 * false, so legitimate playback is untouched. Detected URLs are kept in
 * memory only and exposed through [getDetectedStreams] for the UI to show.
 */
object HlsSnifferPlugin : MnemisWebPlugin {

    override val id = "hls_sniffer"
    override val name = "Stream Sniffer"
    override val version = "v1.0.0"
    override val description = "Optional HLS/MP4 stream detector for compatible web sites."
    override val isBuiltIn = true

    /** Maximum number of detected streams kept in memory (oldest dropped first). */
    const val MAX_STORED_STREAMS = 50

    enum class StreamKind {
        HLS,
        MP4
    }

    data class DetectedStream(
        val url: String,
        val kind: StreamKind,
        val pageUrl: String?,
        val detectedAtMs: Long = System.currentTimeMillis()
    )

    // CopyOnWriteArrayList because shouldBlockRequest runs on the WebView
    // network thread while the UI may read the list at the same time.
    private val detectedStreams = CopyOnWriteArrayList<DetectedStream>()

    override fun matchesUrl(url: String): Boolean = true

    /**
     * Records the request when it points at a direct .m3u8/.mp4 stream.
     * Always returns false: detection only, nothing is ever blocked.
     */
    override fun shouldBlockRequest(url: String): Boolean {
        recordStream(url, pageUrl = null)
        return false
    }

    override fun getUserScript(url: String): String = SNIFFER_JS

    /** Classifies a URL as a direct stream, ignoring query strings and fragments. */
    fun kindOf(url: String): StreamKind? {
        val path = url.substringBefore('?').substringBefore('#').lowercase()
        return when {
            path.endsWith(".m3u8") -> StreamKind.HLS
            path.endsWith(".mp4") -> StreamKind.MP4
            else -> null
        }
    }

    fun isStreamUrl(url: String): Boolean = kindOf(url) != null

    /**
     * Records a stream URL if it is a direct .m3u8/.mp4 link and has not
     * been recorded already. Returns the recorded entry, or null when the
     * URL is not a stream. Safe to call from any thread.
     */
    fun recordStream(url: String, pageUrl: String?): DetectedStream? {
        val kind = kindOf(url) ?: return null
        val cleanUrl = url.trim()
        if (cleanUrl.isEmpty()) return null
        val entry = DetectedStream(url = cleanUrl, kind = kind, pageUrl = pageUrl)
        synchronized(detectedStreams) {
            val existing = detectedStreams.firstOrNull { it.url == cleanUrl }
            if (existing != null) return existing
            if (detectedStreams.size >= MAX_STORED_STREAMS) {
                detectedStreams.removeAt(0)
            }
            detectedStreams.add(entry)
        }
        return entry
    }

    /** Snapshot of all detected streams, oldest first. */
    fun getDetectedStreams(): List<DetectedStream> = detectedStreams.toList()

    fun clearDetectedStreams() {
        detectedStreams.clear()
    }

    private const val SNIFFER_JS = """
        (function() {
            if (window.__mnemisSnifferHooked) return;
            window.__mnemisSnifferHooked = true;
            window.__mnemisDetectedStreams = window.__mnemisDetectedStreams || [];
            var collect = function() {
                var nodes = document.querySelectorAll('video, source');
                for (var i = 0; i < nodes.length; i++) {
                    var src = nodes[i].currentSrc || nodes[i].src || nodes[i].getAttribute('src') || '';
                    if (!src) continue;
                    var clean = src.split('?')[0].split('#')[0].toLowerCase();
                    if (clean.indexOf('.m3u8') === clean.length - 5 ||
                        clean.indexOf('.mp4') === clean.length - 4) {
                        if (window.__mnemisDetectedStreams.indexOf(src) === -1) {
                            window.__mnemisDetectedStreams.push(src);
                        }
                    }
                }
            };
            collect();
            if (window.MutationObserver) {
                new MutationObserver(collect).observe(document.documentElement, {
                    childList: true,
                    subtree: true,
                    attributes: true,
                    attributeFilter: ['src']
                });
            }
        })();
    """
}
