package com.alenia.mnemis.sync

import android.webkit.WebResourceResponse
import java.io.ByteArrayInputStream
import java.net.URI

/**
 * Lightweight network-level ad and tracker filter for WebView playback.
 */
object WebAdBlocker {

    private val blockedHostSuffixes = setOf(
        "doubleclick.net",
        "googlesyndication.com",
        "googleadservices.com",
        "adservice.google.com",
        "imasdk.googleapis.com",
        "adnxs.com",
        "taboola.com",
        "outbrain.com",
        "pubmatic.com",
        "rubiconproject.com",
        "criteo.com",
        "moatads.com",
        "amazon-adsystem.com",
        "popads.net",
        "popcash.net",
        "exoclick.com"
    )

    private val blockedPathSubstrings = listOf(
        "/api/stats/ads",
        "/pagead/",
        "/ptracking",
        "/get_midroll_info",
        "/pcs/activeview",
        "/generate_204"
    )

    fun shouldBlock(rawUrl: String?): Boolean {
        if (rawUrl.isNullOrBlank()) return false
        val uri = try {
            URI(rawUrl)
        } catch (_: Exception) {
            return false
        }
        val host = uri.host?.lowercase() ?: return false
        if (host.contains("googlevideo.com")) return false

        if (blockedHostSuffixes.any { suffix -> host == suffix || host.endsWith(".$suffix") }) {
            return true
        }

        val path = uri.path?.lowercase().orEmpty()
        if (host.contains("youtube.com") || host.contains("youtu.be")) {
            if (blockedPathSubstrings.any { path.contains(it) }) {
                return true
            }
        }
        return false
    }

    fun emptyResponse(): WebResourceResponse {
        return WebResourceResponse("text/plain", "utf-8", ByteArrayInputStream(ByteArray(0)))
    }
}
