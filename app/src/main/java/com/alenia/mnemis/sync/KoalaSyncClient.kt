package com.alenia.mnemis.sync

import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import okhttp3.WebSocket
import okhttp3.WebSocketListener
import org.json.JSONObject
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicLong

/**
 * Native Socket.IO (Engine.IO v4) client over OkHttp for KoalaSync rooms.
 */
class KoalaSyncClient(
    private val serverUrl: String = KoalaSyncProtocol.OFFICIAL_SERVER_URL,
    private val token: String = KoalaSyncProtocol.OFFICIAL_SERVER_TOKEN,
    private val listener: SyncListener
) {

    interface SyncListener {
        fun onRemotePlay(senderId: String, currentTimeSeconds: Double?)
        fun onRemotePause(senderId: String, currentTimeSeconds: Double?)
        fun onRemoteSeek(senderId: String, targetTimeSeconds: Double)
        fun onRemoteDriftCheck(senderId: String, expectedTimeSeconds: Double) {}
        fun onSyncError(message: String)
    }

    enum class ConnectionState {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    }

    data class RoomState(
        val connectionState: ConnectionState = ConnectionState.DISCONNECTED,
        val roomId: String? = null,
        val peerCount: Int = 0,
        val controlMode: String = KoalaSyncProtocol.ControlModes.EVERYONE,
        val isController: Boolean = true
    )

    data class PlaybackSnapshot(
        val isPlaying: Boolean,
        val currentTimeSeconds: Double,
        val mediaTitle: String? = null
    )

    data class ChatMessage(
        val id: String,
        val senderId: String,
        val senderName: String,
        val text: String,
        val timestampMs: Long,
        val isOwn: Boolean
    )

    private val scope = CoroutineScope(SupervisorJob() + Dispatchers.IO)
    private val client = OkHttpClient.Builder()
        .connectTimeout(15, TimeUnit.SECONDS)
        .readTimeout(0, TimeUnit.MILLISECONDS)
        .build()

    private val _roomState = MutableStateFlow(RoomState())
    val roomState: StateFlow<RoomState> = _roomState.asStateFlow()

    private val _chatMessages = MutableStateFlow<List<ChatMessage>>(emptyList())
    val chatMessages: StateFlow<List<ChatMessage>> = _chatMessages.asStateFlow()

    val peerId: String = "mnemis-${UUID.randomUUID().toString().take(8)}"

    private var webSocket: WebSocket? = null
    private var heartbeatJob: Job? = null
    private var reconnectJob: Job? = null

    private var activeRoomId: String? = null
    private var activeUsername: String? = null
    private var activePassword: String? = null
    private var activeMediaTitle: String? = null
    private var activeChatSecret: String? = null
    private var manualDisconnect = false
    private var reconnectAttempt = 0

    private val localSeq = AtomicLong(0L)
    private val lastSeqBySender = ConcurrentHashMap<String, Long>()
    private val activePeers = ConcurrentHashMap.newKeySet<String>()
    private val peerUsernames = ConcurrentHashMap<String, String>()

    @Volatile
    private var currentHostPeerId: String? = null

    @Volatile
    internal var halfRttSeconds: Double = 0.0
        private set

    @Volatile
    private var snapshotProvider: (suspend () -> PlaybackSnapshot)? = null

    fun setSnapshotProvider(provider: (suspend () -> PlaybackSnapshot)?) {
        snapshotProvider = provider
    }

    fun connect(
        roomId: String,
        username: String? = null,
        password: String? = null,
        mediaTitle: String? = null,
        chatSecret: String? = null
    ) {
        disconnect(clearTargetRoom = false)
        manualDisconnect = false
        reconnectAttempt = 0

        activeRoomId = roomId.trim()
        activeUsername = username?.trim()
        activePassword = password
        activeMediaTitle = mediaTitle
        activeChatSecret = chatSecret?.trim()?.takeIf { KoalaSyncChatCrypto.isValidSecret(it) }
            ?: KoalaSyncChatCrypto.generateChatSecret()
        _chatMessages.value = emptyList()

        openWebSocket()
    }

    fun getActiveChatSecret(): String? = activeChatSecret

    fun sendChatMessage(text: String): Boolean {
        val roomId = activeRoomId ?: return false
        val secret = activeChatSecret ?: return false
        val clean = text.trim()
        if (clean.isEmpty() || _roomState.value.connectionState != ConnectionState.CONNECTED) return false
        return try {
            val ciphertext = KoalaSyncChatCrypto.encrypt(clean, roomId, peerId, secret)
            sendEvent(KoalaSyncProtocol.Events.CHAT_MESSAGE, KoalaSyncProtocol.buildChatMessagePayload(ciphertext))
            true
        } catch (_: Exception) {
            false
        }
    }

    fun disconnect(clearTargetRoom: Boolean = true) {
        if (clearTargetRoom) {
            manualDisconnect = true
            activeRoomId = null
            reconnectJob?.cancel()
            reconnectJob = null
        }
        heartbeatJob?.cancel()
        heartbeatJob = null

        webSocket?.let { ws ->
            if (clearTargetRoom && _roomState.value.connectionState == ConnectionState.CONNECTED) {
                ws.send(KoalaSyncProtocol.buildEventFrame(KoalaSyncProtocol.Events.LEAVE_ROOM, JSONObject()))
            }
            ws.close(1000, "Client disconnected")
        }
        webSocket = null
        lastSeqBySender.clear()
        activePeers.clear()

        if (clearTargetRoom) {
            _roomState.value = RoomState()
        } else {
            _roomState.value = _roomState.value.copy(connectionState = ConnectionState.DISCONNECTED)
        }
    }

    fun sendPlay(currentTimeSeconds: Double) {
        if (!canEmitRoomEvents()) return
        val payload = KoalaSyncProtocol.buildMediaPayload(
            peerId = peerId,
            seq = localSeq.incrementAndGet(),
            currentTimeSeconds = currentTimeSeconds
        )
        sendEvent(KoalaSyncProtocol.Events.PLAY, payload)
    }

    fun sendPause(currentTimeSeconds: Double) {
        if (!canEmitRoomEvents()) return
        val payload = KoalaSyncProtocol.buildMediaPayload(
            peerId = peerId,
            seq = localSeq.incrementAndGet(),
            currentTimeSeconds = currentTimeSeconds
        )
        sendEvent(KoalaSyncProtocol.Events.PAUSE, payload)
    }

    fun sendSeek(targetTimeSeconds: Double) {
        if (!canEmitRoomEvents()) return
        val payload = KoalaSyncProtocol.buildMediaPayload(
            peerId = peerId,
            seq = localSeq.incrementAndGet(),
            currentTimeSeconds = targetTimeSeconds,
            targetTimeSeconds = targetTimeSeconds
        )
        sendEvent(KoalaSyncProtocol.Events.SEEK, payload)
    }

    private fun canEmitRoomEvents(): Boolean {
        val state = _roomState.value
        return state.connectionState == ConnectionState.CONNECTED && state.isController
    }

    private fun openWebSocket() {
        val roomId = activeRoomId ?: return
        _roomState.value = _roomState.value.copy(
            connectionState = ConnectionState.CONNECTING,
            roomId = roomId
        )

        val request = Request.Builder()
            .url(KoalaSyncProtocol.buildSocketUrl(serverUrl, token))
            .build()

        webSocket = client.newWebSocket(request, object : WebSocketListener() {
            override fun onOpen(webSocket: WebSocket, response: Response) {
                Log.d(TAG, "WebSocket opened: ${response.code}")
            }

            override fun onMessage(webSocket: WebSocket, text: String) {
                Log.d(TAG, "WS <- $text")
                handleRawMessage(webSocket, text)
            }

            override fun onFailure(webSocket: WebSocket, t: Throwable, response: Response?) {
                val msg = t.message ?: "WebSocket failure (${response?.code ?: "no-resp"})"
                Log.e(TAG, "WS failure: $msg", t)
                handleConnectionDrop(msg)
            }

            override fun onClosed(webSocket: WebSocket, code: Int, reason: String) {
                Log.d(TAG, "WS closed: $code $reason")
                if (!manualDisconnect) {
                    handleConnectionDrop(reason.ifEmpty { "Connection closed ($code)" })
                }
            }
        })
    }

    private fun handleRawMessage(ws: WebSocket, raw: String) {
        when (val packet = KoalaSyncProtocol.parsePacket(raw)) {
            KoalaSyncProtocol.Packet.EngineOpen -> {
                ws.send("40")
            }
            KoalaSyncProtocol.Packet.EnginePing -> {
                ws.send("3")
            }
            KoalaSyncProtocol.Packet.SocketConnectAck -> {
                val roomId = activeRoomId ?: return
                val joinPayload = KoalaSyncProtocol.buildJoinRoomPayload(
                    roomId = roomId,
                    peerId = peerId,
                    username = activeUsername,
                    password = activePassword,
                    mediaTitle = activeMediaTitle
                )
                ws.send(KoalaSyncProtocol.buildEventFrame(KoalaSyncProtocol.Events.JOIN_ROOM, joinPayload))
            }
            is KoalaSyncProtocol.Packet.SocketEvent -> {
                handleSocketEvent(packet.name, packet.data)
            }
            is KoalaSyncProtocol.Packet.Unknown -> Unit
        }
    }

    private fun handleSocketEvent(name: String, data: JSONObject) {
        when (name) {
            KoalaSyncProtocol.Events.ROOM_DATA -> {
                reconnectAttempt = 0
                activePeers.clear()
                val peersArray = data.optJSONArray("peers")
                if (peersArray != null) {
                    for (i in 0 until peersArray.length()) {
                        val peerObj = peersArray.optJSONObject(i) ?: continue
                        val id = peerObj.optString("peerId").orEmpty()
                        if (id.isNotEmpty()) {
                            activePeers.add(id)
                            val uname = peerObj.optString("username", "").takeIf { it.isNotBlank() && it != "null" }
                            if (uname != null) peerUsernames[id] = uname
                        }
                    }
                }
                val mode = data.optString("controlMode", KoalaSyncProtocol.ControlModes.EVERYONE)
                currentHostPeerId = data.optString("hostPeerId", "").ifEmpty { null }
                val isController = computeIsController(data, mode)
                _roomState.value = RoomState(
                    connectionState = ConnectionState.CONNECTED,
                    roomId = data.optString("roomId", activeRoomId.orEmpty()),
                    peerCount = activePeers.size.coerceAtLeast(1),
                    controlMode = mode,
                    isController = isController
                )
                sendLatencyPing()
                startHeartbeat()
            }
            KoalaSyncProtocol.Events.PONG -> {
                val sentAt = data.optLong("t", 0L)
                if (sentAt > 0L) {
                    updateRtt(System.currentTimeMillis() - sentAt)
                }
            }
            KoalaSyncProtocol.Events.CONTROL_MODE -> {
                val mode = data.optString("controlMode", KoalaSyncProtocol.ControlModes.EVERYONE)
                currentHostPeerId = data.optString("hostPeerId", "").ifEmpty { currentHostPeerId }
                _roomState.value = _roomState.value.copy(
                    controlMode = mode,
                    isController = computeIsController(data, mode)
                )
            }
            KoalaSyncProtocol.Events.PEER_STATUS -> {
                val remotePeerId = data.optString("peerId", "")
                val status = data.optString("status", "")
                if (remotePeerId.isNotEmpty()) {
                    if (status == "left") {
                        activePeers.remove(remotePeerId)
                        peerUsernames.remove(remotePeerId)
                    } else {
                        activePeers.add(remotePeerId)
                        val uname = data.optString("username", "").takeIf { it.isNotBlank() && it != "null" }
                        if (uname != null) peerUsernames[remotePeerId] = uname
                    }
                    _roomState.value = _roomState.value.copy(peerCount = activePeers.size.coerceAtLeast(1))

                    val hostId = currentHostPeerId
                    val isAuthoritativePeer = hostId == null || remotePeerId == hostId
                    if (remotePeerId != peerId &&
                        isAuthoritativePeer &&
                        data.optString("playbackState") == "playing" &&
                        data.has("currentTime") &&
                        !data.isNull("currentTime")
                    ) {
                        val expected = (data.optDouble("currentTime") + halfRttSeconds)
                            .coerceIn(0.0, KoalaSyncProtocol.MAX_SYNC_SECONDS)
                        listener.onRemoteDriftCheck(remotePeerId, expected)
                    }
                }
            }
            KoalaSyncProtocol.Events.PLAY -> {
                if (acceptRemoteSequence(data)) {
                    val senderId = data.optString("senderId", "")
                    val currentTime = if (data.has("currentTime") && !data.isNull("currentTime")) {
                        (data.optDouble("currentTime") + halfRttSeconds)
                            .coerceIn(0.0, KoalaSyncProtocol.MAX_SYNC_SECONDS)
                    } else null
                    listener.onRemotePlay(senderId, currentTime)
                }
            }
            KoalaSyncProtocol.Events.PAUSE -> {
                if (acceptRemoteSequence(data)) {
                    val senderId = data.optString("senderId", "")
                    val currentTime = if (data.has("currentTime") && !data.isNull("currentTime")) {
                        data.optDouble("currentTime")
                    } else null
                    listener.onRemotePause(senderId, currentTime)
                }
            }
            KoalaSyncProtocol.Events.SEEK -> {
                if (acceptRemoteSequence(data)) {
                    val senderId = data.optString("senderId", "")
                    val targetTime = when {
                        data.has("targetTime") && !data.isNull("targetTime") -> data.optDouble("targetTime")
                        data.has("currentTime") && !data.isNull("currentTime") -> data.optDouble("currentTime")
                        else -> Double.NaN
                    }
                    if (!targetTime.isNaN()) {
                        listener.onRemoteSeek(senderId, targetTime)
                    }
                }
            }
            KoalaSyncProtocol.Events.FORCE_SYNC_PREPARE -> {
                val senderId = data.optString("senderId", "")
                val targetTime = data.optDouble("targetTime", Double.NaN)
                listener.onRemotePause(senderId, if (targetTime.isNaN()) null else targetTime)
                sendEvent(
                    KoalaSyncProtocol.Events.FORCE_SYNC_ACK,
                    JSONObject().apply {
                        put("peerId", peerId)
                        put("seq", localSeq.incrementAndGet())
                    }
                )
            }
            KoalaSyncProtocol.Events.FORCE_SYNC_EXECUTE -> {
                val senderId = data.optString("senderId", "")
                val targetTime = data.optDouble("targetTime", Double.NaN)
                listener.onRemotePlay(senderId, if (targetTime.isNaN()) null else targetTime)
            }
            KoalaSyncProtocol.Events.CHAT_MESSAGE -> {
                val roomId = activeRoomId ?: return
                val secret = activeChatSecret ?: return
                val senderId = data.optString("senderId", "")
                val ciphertext = data.optString("ciphertext", "")
                if (senderId.isEmpty() || ciphertext.isEmpty()) return
                val decrypted = KoalaSyncChatCrypto.decrypt(ciphertext, roomId, senderId, secret) ?: return
                val msg = ChatMessage(
                    id = data.optString("id", UUID.randomUUID().toString()),
                    senderId = senderId,
                    senderName = peerUsernames[senderId] ?: senderId.take(10),
                    text = decrypted,
                    timestampMs = data.optLong("timestamp", System.currentTimeMillis()),
                    isOwn = senderId == peerId
                )
                _chatMessages.value = (_chatMessages.value + msg).takeLast(100)
            }
            KoalaSyncProtocol.Events.ERROR -> {
                val message = data.optString("message", "Unknown KoalaSync error")
                if (_roomState.value.connectionState == ConnectionState.CONNECTING) {
                    disconnect(clearTargetRoom = true)
                }
                listener.onSyncError(message)
            }
        }
    }

    private fun computeIsController(data: JSONObject, mode: String): Boolean {
        if (mode != KoalaSyncProtocol.ControlModes.HOST_ONLY) return true
        if (data.optString("hostPeerId", "") == peerId) return true
        val controllers = data.optJSONArray("controllers") ?: return false
        for (i in 0 until controllers.length()) {
            if (controllers.optString(i) == peerId) return true
        }
        return false
    }

    /**
     * Drops out-of-order or duplicate events from the same sender.
     */
    internal fun acceptRemoteSequence(data: JSONObject): Boolean {
        val senderId = data.optString("senderId", "")
        if (senderId.isEmpty() || senderId == peerId) return false
        if (!data.has("seq") || data.isNull("seq")) return true
        val seq = data.optLong("seq", -1L)
        if (seq < 0L) return true

        val prev = lastSeqBySender[senderId]
        if (prev != null && seq <= prev) return false
        lastSeqBySender[senderId] = seq
        return true
    }

    internal fun updateRtt(rttMs: Long) {
        if (rttMs < 0L) return
        val oneWaySec = (rttMs / 2.0 / 1000.0).coerceIn(0.0, 1.5)
        halfRttSeconds = if (halfRttSeconds == 0.0) {
            oneWaySec
        } else {
            (halfRttSeconds * 0.6) + (oneWaySec * 0.4)
        }
    }

    private fun sendLatencyPing() {
        sendEvent(
            KoalaSyncProtocol.Events.PING,
            JSONObject().put("t", System.currentTimeMillis())
        )
    }

    private fun startHeartbeat() {
        heartbeatJob?.cancel()
        heartbeatJob = scope.launch {
            while (isActive) {
                delay(KoalaSyncProtocol.HEARTBEAT_INTERVAL_MS)
                sendLatencyPing()
                val snapshot = snapshotProvider?.invoke() ?: continue
                val payload = KoalaSyncProtocol.buildPeerStatusPayload(
                    peerId = peerId,
                    isPlaying = snapshot.isPlaying,
                    currentTimeSeconds = snapshot.currentTimeSeconds,
                    mediaTitle = snapshot.mediaTitle ?: activeMediaTitle,
                    username = activeUsername
                )
                sendEvent(KoalaSyncProtocol.Events.PEER_STATUS, payload)
            }
        }
    }

    private fun handleConnectionDrop(reason: String) {
        heartbeatJob?.cancel()
        heartbeatJob = null
        webSocket = null

        if (manualDisconnect || activeRoomId == null) {
            _roomState.value = RoomState()
            return
        }

        _roomState.value = _roomState.value.copy(connectionState = ConnectionState.DISCONNECTED)
        scheduleReconnect(reason)
    }

    private fun scheduleReconnect(reason: String) {
        if (reconnectAttempt == 0) {
            listener.onSyncError(reason)
        }
        if (reconnectJob?.isActive == true) return
        val delayMs = (1000L shl reconnectAttempt.coerceAtMost(4)).coerceAtMost(16_000L)
        reconnectAttempt++
        reconnectJob = scope.launch {
            delay(delayMs)
            if (!manualDisconnect && activeRoomId != null) {
                openWebSocket()
            }
        }
    }

    private fun sendEvent(eventName: String, payload: JSONObject) {
        val frame = KoalaSyncProtocol.buildEventFrame(eventName, payload)
        Log.d(TAG, "WS -> $frame")
        webSocket?.send(frame)
    }

    companion object {
        private const val TAG = "KoalaSyncClient"
    }
}

