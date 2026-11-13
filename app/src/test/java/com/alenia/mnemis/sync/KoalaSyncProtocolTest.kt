package com.alenia.mnemis.sync

import com.alenia.mnemis.plugin.MnemisPlugin
import com.alenia.mnemis.plugin.MnemisPluginRegistry
import org.json.JSONObject
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

class KoalaSyncProtocolTest {

    @Test
    fun parsePacket_recognizesEngineIoAndSocketIoFrames() {
        assertEquals(KoalaSyncProtocol.Packet.EngineOpen, KoalaSyncProtocol.parsePacket("0{\"sid\":\"abc\"}"))
        assertEquals(KoalaSyncProtocol.Packet.EnginePing, KoalaSyncProtocol.parsePacket("2"))
        assertEquals(KoalaSyncProtocol.Packet.SocketConnectAck, KoalaSyncProtocol.parsePacket("40{\"sid\":\"xyz\"}"))

        val event = KoalaSyncProtocol.parsePacket("42[\"seek\",{\"targetTime\":125.5,\"seq\":3}]")
        assertTrue(event is KoalaSyncProtocol.Packet.SocketEvent)
        val socketEvent = event as KoalaSyncProtocol.Packet.SocketEvent
        assertEquals("seek", socketEvent.name)
        assertEquals(125.5, socketEvent.data.getDouble("targetTime"), 0.001)
        assertEquals(3L, socketEvent.data.getLong("seq"))
    }

    @Test
    fun buildJoinRoomPayload_includesRequiredProtocolVersion() {
        val payload = KoalaSyncProtocol.buildJoinRoomPayload(
            roomId = "room-1",
            peerId = "mnemis-1234",
            username = "Alex"
        )
        assertEquals("room-1", payload.getString("roomId"))
        assertEquals("mnemis-1234", payload.getString("peerId"))
        assertEquals(KoalaSyncProtocol.PROTOCOL_VERSION, payload.getString("protocolVersion"))
        assertEquals("Alex", payload.getString("username"))
    }

    @Test
    fun buildMediaPayload_clampsOutOfRangeSeconds() {
        val negative = KoalaSyncProtocol.buildMediaPayload("p1", 1L, -5.0, targetTimeSeconds = 999999.0)
        assertEquals(0.0, negative.getDouble("currentTime"), 0.001)
        assertEquals(KoalaSyncProtocol.MAX_SYNC_SECONDS, negative.getDouble("targetTime"), 0.001)
    }

    @Test
    fun acceptRemoteSequence_filtersSelfAndStaleEvents() {
        val noopListener = object : KoalaSyncClient.SyncListener {
            override fun onRemotePlay(senderId: String, currentTimeSeconds: Double?) = Unit
            override fun onRemotePause(senderId: String, currentTimeSeconds: Double?) = Unit
            override fun onRemoteSeek(senderId: String, targetTimeSeconds: Double) = Unit
            override fun onSyncError(message: String) = Unit
        }
        val client = KoalaSyncClient(listener = noopListener)

        val selfEvent = JSONObject().put("senderId", client.peerId).put("seq", 10L)
        assertFalse(client.acceptRemoteSequence(selfEvent))

        val firstRemote = JSONObject().put("senderId", "peer-2").put("seq", 5L)
        assertTrue(client.acceptRemoteSequence(firstRemote))

        val staleRemote = JSONObject().put("senderId", "peer-2").put("seq", 5L)
        assertFalse(client.acceptRemoteSequence(staleRemote))

        val olderRemote = JSONObject().put("senderId", "peer-2").put("seq", 3L)
        assertFalse(client.acceptRemoteSequence(olderRemote))

        val newerRemote = JSONObject().put("senderId", "peer-2").put("seq", 6L)
        assertTrue(client.acceptRemoteSequence(newerRemote))
    }

    @Test
    fun updateRtt_computesAndClampsHalfRttSeconds() {
        val noopListener = object : KoalaSyncClient.SyncListener {
            override fun onRemotePlay(senderId: String, currentTimeSeconds: Double?) = Unit
            override fun onRemotePause(senderId: String, currentTimeSeconds: Double?) = Unit
            override fun onRemoteSeek(senderId: String, targetTimeSeconds: Double) = Unit
            override fun onSyncError(message: String) = Unit
        }
        val client = KoalaSyncClient(listener = noopListener)

        client.updateRtt(400L)
        assertEquals(0.2, client.halfRttSeconds, 0.001)

        client.updateRtt(10_000L)
        // EMA: 0.2 * 0.6 + 1.5 (clamped) * 0.4 = 0.72
        assertEquals(0.72, client.halfRttSeconds, 0.001)
    }

    @Test
    fun webAdBlocker_blocksAdDomainsAndPathsButAllowsVideoStreams() {
        assertTrue(WebAdBlocker.shouldBlock("https://googleads.g.doubleclick.net/pagead/id"))
        assertTrue(WebAdBlocker.shouldBlock("https://www.youtube.com/api/stats/ads?ns=yt"))
        assertTrue(WebAdBlocker.shouldBlock("https://m.youtube.com/pagead/lvz"))
        assertFalse(WebAdBlocker.shouldBlock("https://rr3---sn-4g5e6ns7.googlevideo.com/videoplayback?id=123"))
        assertFalse(WebAdBlocker.shouldBlock("https://m.youtube.com/watch?v=dQw4w9WgXcQ"))
    }

    @Test
    fun chatCrypto_encryptsAndDecryptsRoundTripAndRejectsWrongSecret() {
        val roomId = "room-42"
        val senderId = "mnemis-1234"
        val secret = KoalaSyncChatCrypto.generateChatSecret()
        assertTrue(KoalaSyncChatCrypto.isValidSecret(secret))
        val message = "Hola sala! 🔥"

        val encrypted = KoalaSyncChatCrypto.encrypt(
            text = message,
            roomId = roomId,
            senderId = senderId,
            secret = secret
        )
        val decrypted = KoalaSyncChatCrypto.decrypt(
            ciphertextBase64Url = encrypted,
            roomId = roomId,
            senderId = senderId,
            secret = secret
        )
        assertEquals(message, decrypted)

        val wrongSecret = KoalaSyncChatCrypto.decrypt(
            ciphertextBase64Url = encrypted,
            roomId = roomId,
            senderId = senderId,
            secret = KoalaSyncChatCrypto.generateChatSecret()
        )
        assertNull(wrongSecret)
    }

    @Test
    fun pluginRegistry_allowsCustomPluginRegistrationAndUrlMatching() {
        val customPlugin = object : MnemisPlugin {
            override val id = "test_custom_plugin"
            override val name = "Test Custom"
            override val version = "1.0.0"
            override val description = "Custom test plugin"
        }

        MnemisPluginRegistry.register(customPlugin)
        assertTrue(MnemisPluginRegistry.allPlugins().any { it.id == "test_custom_plugin" })

        val matchedYouTube = MnemisPluginRegistry.findMatchingWebPlugins("https://m.youtube.com/watch?v=123")
        assertTrue(matchedYouTube.any { it.id == "youtube_spa" })
        assertTrue(matchedYouTube.any { it.id == "universal_media_sync" })

        MnemisPluginRegistry.unregister("test_custom_plugin")
        assertFalse(MnemisPluginRegistry.allPlugins().any { it.id == "test_custom_plugin" })
    }
}

