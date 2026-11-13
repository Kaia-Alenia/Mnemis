package com.alenia.mnemis.prefs

import android.content.Context
import android.content.SharedPreferences
import com.alenia.mnemis.sync.KoalaSyncProtocol

/**
 * Persisted user preferences for Mnemis plugins, sync defaults, and WebView behavior.
 */
class MnemisPreferences(context: Context) {

    private val prefs: SharedPreferences =
        context.applicationContext.getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE)

    var defaultUsername: String
        get() = prefs.getString(KEY_USERNAME, "").orEmpty()
        set(value) = prefs.edit().putString(KEY_USERNAME, value.trim()).apply()

    var relayServerUrl: String
        get() = prefs.getString(KEY_SERVER_URL, KoalaSyncProtocol.OFFICIAL_SERVER_URL)
            ?.takeIf { it.isNotBlank() } ?: KoalaSyncProtocol.OFFICIAL_SERVER_URL
        set(value) = prefs.edit().putString(
            KEY_SERVER_URL,
            value.trim().ifEmpty { KoalaSyncProtocol.OFFICIAL_SERVER_URL }
        ).apply()

    var adBlockEnabled: Boolean
        get() = prefs.getBoolean(KEY_ADBLOCK_ENABLED, true)
        set(value) = prefs.edit().putBoolean(KEY_ADBLOCK_ENABLED, value).apply()

    var koalaSyncPluginEnabled: Boolean
        get() = prefs.getBoolean(KEY_KOALASYNC_ENABLED, true)
        set(value) = prefs.edit().putBoolean(KEY_KOALASYNC_ENABLED, value).apply()

    fun isPluginInstalled(pluginId: String): Boolean {
        if (pluginId == PLUGIN_KOALASYNC || pluginId == PLUGIN_ADBLOCK || pluginId == PLUGIN_YOUTUBE_SPA) {
            return true
        }
        return prefs.getBoolean("plugin_installed_$pluginId", false)
    }

    fun setPluginInstalled(pluginId: String, installed: Boolean) {
        prefs.edit().putBoolean("plugin_installed_$pluginId", installed).apply()
    }

    fun isPluginEnabled(pluginId: String): Boolean {
        return when (pluginId) {
            PLUGIN_ADBLOCK -> adBlockEnabled
            PLUGIN_KOALASYNC -> koalaSyncPluginEnabled
            else -> prefs.getBoolean("plugin_enabled_$pluginId", true)
        }
    }

    fun setPluginEnabled(pluginId: String, enabled: Boolean) {
        when (pluginId) {
            PLUGIN_ADBLOCK -> adBlockEnabled = enabled
            PLUGIN_KOALASYNC -> koalaSyncPluginEnabled = enabled
            else -> prefs.edit().putBoolean("plugin_enabled_$pluginId", enabled).apply()
        }
    }

    data class LastPlayedItem(
        val title: String,
        val source: String,
        val isWeb: Boolean,
        val timestampMs: Long
    )

    fun saveLastPlayed(title: String, source: String, isWeb: Boolean) {
        prefs.edit()
            .putString(KEY_LAST_TITLE, title)
            .putString(KEY_LAST_SOURCE, source)
            .putBoolean(KEY_LAST_IS_WEB, isWeb)
            .putLong(KEY_LAST_TS, System.currentTimeMillis())
            .apply()
    }

    fun getLastPlayed(): LastPlayedItem? {
        val source = prefs.getString(KEY_LAST_SOURCE, null)?.takeIf { it.isNotBlank() } ?: return null
        val title = prefs.getString(KEY_LAST_TITLE, null)?.takeIf { it.isNotBlank() } ?: source
        return LastPlayedItem(
            title = title,
            source = source,
            isWeb = prefs.getBoolean(KEY_LAST_IS_WEB, false),
            timestampMs = prefs.getLong(KEY_LAST_TS, 0L)
        )
    }

    companion object {
        private const val PREFS_NAME = "mnemis_prefs"
        private const val KEY_USERNAME = "default_username"
        private const val KEY_SERVER_URL = "relay_server_url"
        private const val KEY_ADBLOCK_ENABLED = "adblock_enabled"
        private const val KEY_KOALASYNC_ENABLED = "koalasync_enabled"
        private const val KEY_LAST_TITLE = "last_played_title"
        private const val KEY_LAST_SOURCE = "last_played_source"
        private const val KEY_LAST_IS_WEB = "last_played_is_web"
        private const val KEY_LAST_TS = "last_played_ts"

        const val PLUGIN_KOALASYNC = "koalasync"
        const val PLUGIN_ADBLOCK = "web_adblock"
        const val PLUGIN_YOUTUBE_SPA = "youtube_spa"
    }
}

