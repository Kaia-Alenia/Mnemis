package com.alenia.mnemis.sync

import org.json.JSONArray
import org.json.JSONObject

/**
 * Wire protocol constants, packet framing, and payload helpers for KoalaSync (Engine.IO v4 / Socket.IO).
 */
object KoalaSyncProtocol {

    const val PROTOCOL_VERSION = "1.0.0"
    const val APP_VERSION = "3.1.4"
    const val OFFICIAL_SERVER_URL = "wss://syncserver.koalastuff.net"
    const val OFFICIAL_SERVER_TOKEN =
        "62170b705234c4f4807a9b22420bb93cf1a2aacfa4c5d3b47804482babb8eb50"

    const val HEARTBEAT_INTERVAL_MS = 15_000L
    const val MAX_SYNC_SECONDS = 86_400.0

    object Events {
        const val JOIN_ROOM = "join_room"
        const val LEAVE_ROOM = "leave_room"
        const val ROOM_DATA = "room_data"
        const val ERROR = "error"

        const val CONTROL_MODE = "control_mode"
        const val PLAY = "play"
        const val PAUSE = "pause"
        const val SEEK = "seek"

        const val PEER_STATUS = "peer_status"
        const val FORCE_SYNC_PREPARE = "force_sync_prepare"
        const val FORCE_SYNC_ACK = "force_sync_ack"
        const val FORCE_SYNC_EXECUTE = "force_sync_execute"
        const val CHAT_MESSAGE = "chat_message"
        const val PING = "ping"
        const val PONG = "pong"
    }

    val CLIENT_CAPABILITIES = listOf("host-control", "co-host", "chat", "chat-v1")

    object ControlModes {
        const val EVERYONE = "everyone"
        const val HOST_ONLY = "host-only"
    }

    sealed class Packet {
        data object EngineOpen : Packet()
        data object EnginePing : Packet()
        data object SocketConnectAck : Packet()
        data class SocketEvent(val name: String, val data: JSONObject) : Packet()
        data class Unknown(val raw: String) : Packet()
    }

    fun buildSocketUrl(serverUrl: String, token: String): String {
        val base = serverUrl.trimEnd('/')
        return "$base/socket.io/?EIO=4&transport=websocket&version=$APP_VERSION&token=$token"
    }

    fun parsePacket(raw: String): Packet {
        return when {
            raw == "2" -> Packet.EnginePing
            raw.startsWith("0") -> Packet.EngineOpen
            raw.startsWith("40") -> Packet.SocketConnectAck
            raw.startsWith("42") -> parseSocketEvent(raw.substring(2))
            else -> Packet.Unknown(raw)
        }
    }

    private fun parseSocketEvent(jsonBody: String): Packet {
        return try {
            val array = JSONArray(jsonBody)
            val eventName = array.optString(0, "")
            if (eventName.isEmpty()) {
                Packet.Unknown("42$jsonBody")
            } else {
                val payload = array.optJSONObject(1) ?: JSONObject()
                Packet.SocketEvent(eventName, payload)
            }
        } catch (_: Exception) {
            Packet.Unknown("42$jsonBody")
        }
    }

    fun buildEventFrame(eventName: String, payload: JSONObject): String {
        val array = JSONArray()
        array.put(eventName)
        array.put(payload)
        return "42$array"
    }

    fun buildJoinRoomPayload(
        roomId: String,
        peerId: String,
        username: String? = null,
        password: String? = null,
        mediaTitle: String? = null
    ): JSONObject {
        return JSONObject().apply {
            put("roomId", roomId)
            put("peerId", peerId)
            put("protocolVersion", PROTOCOL_VERSION)
            if (!username.isNullOrBlank()) put("username", username.take(30))
            if (!password.isNullOrBlank()) put("password", password.take(128))
            if (!mediaTitle.isNullOrBlank()) put("mediaTitle", mediaTitle.take(100))
            put("clientCapabilities", JSONArray(CLIENT_CAPABILITIES))
        }
    }

    fun buildChatMessagePayload(ciphertext: String): JSONObject {
        return JSONObject().apply {
            put("v", 1)
            put("ciphertext", ciphertext)
        }
    }

    fun buildMediaPayload(
        peerId: String,
        seq: Long,
        currentTimeSeconds: Double,
        targetTimeSeconds: Double? = null,
        actionTimestampMs: Long = System.currentTimeMillis()
    ): JSONObject {
        val clampedCurrent = currentTimeSeconds.coerceIn(0.0, MAX_SYNC_SECONDS)
        return JSONObject().apply {
            put("peerId", peerId)
            put("seq", seq)
            put("currentTime", clampedCurrent)
            if (targetTimeSeconds != null) {
                put("targetTime", targetTimeSeconds.coerceIn(0.0, MAX_SYNC_SECONDS))
            }
            put("actionTimestamp", actionTimestampMs)
        }
    }

    fun buildPeerStatusPayload(
        peerId: String,
        isPlaying: Boolean,
        currentTimeSeconds: Double,
        mediaTitle: String? = null,
        username: String? = null
    ): JSONObject {
        return JSONObject().apply {
            put("peerId", peerId)
            put("playbackState", if (isPlaying) "playing" else "paused")
            put("currentTime", currentTimeSeconds.coerceIn(0.0, MAX_SYNC_SECONDS))
            if (!mediaTitle.isNullOrBlank()) put("mediaTitle", mediaTitle.take(100))
            if (!username.isNullOrBlank()) put("username", username.take(30))
        }
    }
}

