package com.alenia.mnemis.model

/**
 * Represents an installable or built-in Mnemis plugin module.
 */
data class PluginItem(
    val id: String,
    val name: String,
    val version: String,
    val description: String,
    val isBuiltIn: Boolean = false,
    val isInstalled: Boolean = true,
    val isEnabled: Boolean = true
)

