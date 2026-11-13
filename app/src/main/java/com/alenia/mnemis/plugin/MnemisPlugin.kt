package com.alenia.mnemis.plugin

/**
 * Base contract for any Mnemis plugin module (built-in or community-contributed).
 */
interface MnemisPlugin {
    val id: String
    val name: String
    val version: String
    val description: String
    val isBuiltIn: Boolean get() = false
}

/**
 * Extension point for WebView-based streaming platforms.
 * Community plugins can match target URLs, inject custom JS bridges, or filter network requests.
 */
interface MnemisWebPlugin : MnemisPlugin {
    fun matchesUrl(url: String): Boolean
    fun getUserScript(url: String): String? = null
    fun shouldBlockRequest(url: String): Boolean = false
}

