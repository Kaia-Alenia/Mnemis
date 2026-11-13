package com.alenia.mnemis.update

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONObject
import java.util.concurrent.TimeUnit

/**
 * Checks GitHub Releases for newer Mnemis versions and returns download metadata.
 */
object MnemisUpdater {

    const val GITHUB_LATEST_RELEASE_API =
        "https://api.github.com/repos/Kaia-Alenia/Mnemis/releases/latest"

    data class UpdateInfo(
        val latestVersion: String,
        val releaseNotes: String,
        val downloadUrl: String,
        val htmlUrl: String
    )

    private val httpClient: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .connectTimeout(10, TimeUnit.SECONDS)
            .readTimeout(12, TimeUnit.SECONDS)
            .build()
    }
    suspend fun checkForUpdates(currentVersion: String): Result<UpdateInfo?> =
        withContext(Dispatchers.IO) {
            runCatching {
                val request = Request.Builder()
                    .url(GITHUB_LATEST_RELEASE_API)
                    .header("Accept", "application/vnd.github+json")
                    .header("User-Agent", "Mnemis-Android")
                    .build()

                httpClient.newCall(request).execute().use { response ->
                    if (!response.isSuccessful) {
                        error("HTTP ${response.code}")
                    }
                    val body = response.body?.string().orEmpty()
                    parseReleaseJson(body, currentVersion)
                }
            }
        }

    fun parseReleaseJson(rawJson: String, currentVersion: String): UpdateInfo? {
        if (rawJson.isBlank()) return null
        return try {
            val obj = JSONObject(rawJson)
            if (obj.optBoolean("draft", false) || obj.optBoolean("prerelease", false)) {
                return null
            }
            val rawTag = obj.optString("tag_name", "").trim()
            if (rawTag.isEmpty() || !isNewerVersion(rawTag, currentVersion)) {
                return null
            }

            val htmlUrl = obj.optString("html_url", "").trim()
            if (htmlUrl.isEmpty()) return null

            var apkUrl = ""
            val assets = obj.optJSONArray("assets")
            if (assets != null) {
                for (i in 0 until assets.length()) {
                    val asset = assets.optJSONObject(i) ?: continue
                    val name = asset.optString("name", "")
                    val url = asset.optString("browser_download_url", "")
                    if (name.endsWith(".apk", ignoreCase = true) && url.isNotBlank()) {
                        apkUrl = url
                        break
                    }
                }
            }

            UpdateInfo(
                latestVersion = normalizeVersion(rawTag),
                releaseNotes = obj.optString("body", "").trim(),
                downloadUrl = apkUrl.ifEmpty { htmlUrl },
                htmlUrl = htmlUrl
            )
        } catch (_: Exception) {
            null
        }
    }

    fun isNewerVersion(latest: String, current: String): Boolean {
        val latestParts = parseNumericParts(latest)
        val currentParts = parseNumericParts(current)
        val maxLen = maxOf(latestParts.size, currentParts.size)
        for (i in 0 until maxLen) {
            val l = latestParts.getOrElse(i) { 0 }
            val c = currentParts.getOrElse(i) { 0 }
            if (l > c) return true
            if (l < c) return false
        }
        return false
    }

    private fun normalizeVersion(version: String): String {
        return version.trim().removePrefix("v").removePrefix("V")
    }

    private fun parseNumericParts(version: String): List<Int> {
        val clean = normalizeVersion(version).substringBefore("-")
        return clean.split(".")
            .mapNotNull { part -> part.filter { it.isDigit() }.toIntOrNull() }
    }
}
