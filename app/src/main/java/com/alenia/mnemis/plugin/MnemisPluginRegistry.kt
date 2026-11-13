package com.alenia.mnemis.plugin

import com.alenia.mnemis.prefs.MnemisPreferences
import com.alenia.mnemis.sync.WebAdBlocker
import java.util.concurrent.CopyOnWriteArrayList

/**
 * Central registry for Mnemis built-in and third-party plugins.
 */
object MnemisPluginRegistry {

    private val plugins = CopyOnWriteArrayList<MnemisPlugin>()

    init {
        register(
            object : MnemisPlugin {
                override val id = MnemisPreferences.PLUGIN_KOALASYNC
                override val name = "KoalaSync"
                override val version = "v3.1.4"
                override val description = "Real-time room sync and E2E chat for local and web playback."
                override val isBuiltIn = true
            }
        )
        register(
            object : MnemisWebPlugin {
                override val id = MnemisPreferences.PLUGIN_ADBLOCK
                override val name = "Web AdBlocker"
                override val version = "v1.0.0"
                override val description = "Blocks network ad trackers and YouTube pre-roll placements."
                override val isBuiltIn = true
                override fun matchesUrl(url: String): Boolean = true
                override fun shouldBlockRequest(url: String): Boolean = WebAdBlocker.shouldBlock(url)
            }
        )
        register(
            object : MnemisWebPlugin {
                override val id = MnemisPreferences.PLUGIN_YOUTUBE_SPA
                override val name = "YouTube SPA Bridge"
                override val version = "v1.1.0"
                override val description = "Handles YouTube SPA navigation and #movie_player controls."
                override val isBuiltIn = true
                override fun matchesUrl(url: String): Boolean =
                    url.contains("youtube.com", ignoreCase = true) ||
                        url.contains("youtu.be", ignoreCase = true)
            }
        )
        register(
            object : MnemisWebPlugin {
                override val id = "universal_media_sync"
                override val name = "Universal Web Players"
                override val version = "v1.0.0"
                override val description = "Extends sync support to Twitch, Vimeo, JWPlayer, VideoJS and Shadow DOM."
                override val isBuiltIn = true
                override fun matchesUrl(url: String): Boolean = true
                override fun getUserScript(url: String): String = UNIVERSAL_PLAYERS_JS
            }
        )
    }

    fun register(plugin: MnemisPlugin) {
        if (plugins.none { it.id == plugin.id }) {
            plugins.add(plugin)
        }
    }

    fun unregister(pluginId: String) {
        plugins.removeAll { it.id == pluginId && !it.isBuiltIn }
    }

    fun allPlugins(): List<MnemisPlugin> = plugins.toList()

    fun findMatchingWebPlugins(url: String, prefs: MnemisPreferences? = null): List<MnemisWebPlugin> {
        return plugins.filterIsInstance<MnemisWebPlugin>().filter { plugin ->
            (prefs?.isPluginEnabled(plugin.id) ?: true) && plugin.matchesUrl(url)
        }
    }

    private val UNIVERSAL_PLAYERS_JS = """
        (function() {
            if (window.__mnemisUniversalHooked) return;
            window.__mnemisUniversalHooked = true;
            // Expose Shadow DOM video lookup helper for KoalaSyncWebBridge
            window.__mnemisCollectDeepVideos = function(root) {
                const found = [];
                const walk = (node) => {
                    if (!node) return;
                    if (node.tagName === 'VIDEO') found.push(node);
                    if (node.shadowRoot) walk(node.shadowRoot);
                    const children = node.children || [];
                    for (let i = 0; i < children.length; i++) walk(children[i]);
                };
                walk(root || document.documentElement);
                return found;
            };
        })();
    """.trimIndent()
}

