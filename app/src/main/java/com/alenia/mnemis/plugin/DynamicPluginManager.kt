package com.alenia.mnemis.plugin

import android.content.Context
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.util.concurrent.TimeUnit

/**
 * Loads, downloads, and persists declarative JSON web plugins into filesDir/mnemis_plugins
 * so new platform integrations can be installed without recompiling the APK.
 */
object DynamicPluginManager {

    private const val PLUGINS_DIR = "mnemis_plugins"
    private val httpClient: OkHttpClient by lazy {
        OkHttpClient.Builder()
            .connectTimeout(12, TimeUnit.SECONDS)
            .readTimeout(15, TimeUnit.SECONDS)
            .build()
    }

    fun getPluginsDir(context: Context): File {
        val dir = File(context.filesDir, PLUGINS_DIR)
        if (!dir.exists()) {
            dir.mkdirs()
        }
        return dir
    }

    fun loadAll(context: Context): List<MnemisWebPlugin> {
        val files = getPluginsDir(context).listFiles { file ->
            file.isFile && file.extension.equals("json", ignoreCase = true)
        } ?: return emptyList()

        val loaded = mutableListOf<MnemisWebPlugin>()
        for (file in files) {
            val plugin = runCatching { parseManifest(file.readText()) }.getOrNull()
            if (plugin != null) {
                MnemisPluginRegistry.register(plugin)
                loaded.add(plugin)
            }
        }
        return loaded
    }

    suspend fun fetchAndInstall(context: Context, manifestUrl: String): Result<MnemisWebPlugin> =
        withContext(Dispatchers.IO) {
            runCatching {
                val request = Request.Builder()
                    .url(manifestUrl.trim())
                    .header("User-Agent", "Mnemis-Android")
                    .build()
                httpClient.newCall(request).execute().use { response ->
                    if (!response.isSuccessful) {
                        error("HTTP ${response.code}")
                    }
                    val body = response.body?.string().orEmpty()
                    val plugin = parseManifest(body)
                        ?: error("Invalid plugin manifest")
                    val safeFileName = plugin.id.replace(Regex("[^a-zA-Z0-9._-]"), "_")
                    File(getPluginsDir(context), "$safeFileName.json").writeText(body)
                    MnemisPluginRegistry.register(plugin)
                    plugin
                }
            }
        }

    fun uninstall(context: Context, pluginId: String): Boolean {
        MnemisPluginRegistry.unregister(pluginId)
        val safeFileName = pluginId.replace(Regex("[^a-zA-Z0-9._-]"), "_")
        val file = File(getPluginsDir(context), "$safeFileName.json")
        return if (file.exists()) file.delete() else false
    }

    fun parseManifest(rawJson: String): MnemisWebPlugin? {
        if (rawJson.isBlank()) return null
        return try {
            val obj = JSONObject(rawJson)
            val id = obj.optString("id", "").trim()
            val name = obj.optString("name", "").trim()
            if (id.isEmpty() || name.isEmpty()) return null

            val version = obj.optString("version", "1.0.0").trim().ifEmpty { "1.0.0" }
            val description = obj.optString("description", "").trim()
            val matchDomains = jsonArrayToList(obj.optJSONArray("matchDomains"))
            val blockPatterns = jsonArrayToList(obj.optJSONArray("blockPatterns"))
            val userScript = obj.optString("userScript", "").takeIf { it.isNotBlank() }

            object : MnemisWebPlugin {
                override val id: String = id
                override val name: String = name
                override val version: String = version
                override val description: String = description
                override val isBuiltIn: Boolean = false

                override fun matchesUrl(url: String): Boolean {
                    if (matchDomains.isEmpty() || matchDomains.contains("*")) return true
                    return matchDomains.any { domain ->
                        url.contains(domain, ignoreCase = true)
                    }
                }

                override fun shouldBlockRequest(url: String): Boolean {
                    if (blockPatterns.isEmpty()) return false
                    return blockPatterns.any { pattern ->
                        url.contains(pattern, ignoreCase = true)
                    }
                }

                override fun getUserScript(url: String): String? = userScript
            }
        } catch (_: Exception) {
            null
        }
    }

    private fun jsonArrayToList(array: JSONArray?): List<String> {
        if (array == null) return emptyList()
        val result = mutableListOf<String>()
        for (i in 0 until array.length()) {
            val value = array.optString(i, "").trim()
            if (value.isNotEmpty()) {
                result.add(value)
            }
        }
        return result
    }
}
