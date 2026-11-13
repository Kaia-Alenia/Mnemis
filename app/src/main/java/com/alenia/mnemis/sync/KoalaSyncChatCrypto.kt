package com.alenia.mnemis.sync

import java.nio.ByteBuffer
import java.security.SecureRandom
import javax.crypto.Cipher
import javax.crypto.Mac
import javax.crypto.spec.GCMParameterSpec
import javax.crypto.spec.SecretKeySpec

/**
 * End-to-end encryption helper compatible with KoalaSync's WebCrypto chat-crypto.js
 * (HKDF-SHA256 + AES-256-GCM with AAD = "roomId|senderId").
 */
object KoalaSyncChatCrypto {

    private const val CHAT_INFO = "KoalaSync E2E Chat v1"
    private const val SECRET_BYTES = 16
    private const val IV_BYTES = 12
    private const val GCM_TAG_BITS = 128

    private val random = SecureRandom()

    fun generateChatSecret(): String {
        val bytes = ByteArray(SECRET_BYTES)
        random.nextBytes(bytes)
        return encodeBase64Url(bytes)
    }

    fun isValidSecret(secret: String?): Boolean {
        if (secret.isNullOrBlank()) return false
        return try {
            val decoded = decodeBase64Url(secret)
            decoded?.size == SECRET_BYTES
        } catch (_: Exception) {
            false
        }
    }

    fun encrypt(text: String, roomId: String, senderId: String, secret: String): String {
        val key = deriveKey(roomId, secret)
        val iv = ByteArray(IV_BYTES)
        random.nextBytes(iv)

        val cipher = Cipher.getInstance("AES/GCM/NoPadding")
        cipher.init(Cipher.ENCRYPT_MODE, key, GCMParameterSpec(GCM_TAG_BITS, iv))
        cipher.updateAAD("$roomId|$senderId".toByteArray(Charsets.UTF_8))

        val ciphertext = cipher.doFinal(text.trim().toByteArray(Charsets.UTF_8))
        val combined = ByteBuffer.allocate(iv.size + ciphertext.size)
            .put(iv)
            .put(ciphertext)
            .array()
        return encodeBase64Url(combined)
    }

    fun decrypt(ciphertextBase64Url: String, roomId: String, senderId: String, secret: String): String? {
        return try {
            val combined = decodeBase64Url(ciphertextBase64Url) ?: return null
            if (combined.size <= IV_BYTES + 16) return null

            val iv = combined.copyOfRange(0, IV_BYTES)
            val encrypted = combined.copyOfRange(IV_BYTES, combined.size)
            val key = deriveKey(roomId, secret)

            val cipher = Cipher.getInstance("AES/GCM/NoPadding")
            cipher.init(Cipher.DECRYPT_MODE, key, GCMParameterSpec(GCM_TAG_BITS, iv))
            cipher.updateAAD("$roomId|$senderId".toByteArray(Charsets.UTF_8))

            String(cipher.doFinal(encrypted), Charsets.UTF_8)
        } catch (_: Exception) {
            null
        }
    }

    private fun deriveKey(roomId: String, secret: String): SecretKeySpec {
        val ikm = decodeBase64Url(secret) ?: throw IllegalArgumentException("Invalid chat secret")
        val salt = roomId.toByteArray(Charsets.UTF_8)
        val info = CHAT_INFO.toByteArray(Charsets.UTF_8)

        // HKDF-Extract: PRK = HMAC-SHA256(salt, IKM)
        val extractMac = Mac.getInstance("HmacSHA256")
        extractMac.init(SecretKeySpec(salt, "HmacSHA256"))
        val prk = extractMac.doFinal(ikm)

        // HKDF-Expand (32 bytes = single SHA-256 block with counter 0x01)
        val expandMac = Mac.getInstance("HmacSHA256")
        expandMac.init(SecretKeySpec(prk, "HmacSHA256"))
        expandMac.update(info)
        expandMac.update(0x01.toByte())
        val okm = expandMac.doFinal()

        return SecretKeySpec(okm, "AES")
    }

    private const val B64_ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"

    private fun encodeBase64Url(data: ByteArray): String {
        val out = StringBuilder((data.size * 4 + 2) / 3)
        var i = 0
        while (i < data.size) {
            val b0 = data[i].toInt() and 0xFF
            val hasB1 = i + 1 < data.size
            val b1 = if (hasB1) data[i + 1].toInt() and 0xFF else 0
            val hasB2 = i + 2 < data.size
            val b2 = if (hasB2) data[i + 2].toInt() and 0xFF else 0

            out.append(B64_ALPHABET[b0 ushr 2])
            out.append(B64_ALPHABET[((b0 and 0x03) shl 4) or (b1 ushr 4)])
            if (hasB1) out.append(B64_ALPHABET[((b1 and 0x0F) shl 2) or (b2 ushr 6)])
            if (hasB2) out.append(B64_ALPHABET[b2 and 0x3F])
            i += 3
        }
        return out.toString()
    }

    private fun decodeBase64Url(input: String): ByteArray? {
        if (input.isEmpty() || input.length % 4 == 1) return null
        val values = IntArray(input.length)
        for (idx in input.indices) {
            val pos = B64_ALPHABET.indexOf(input[idx])
            if (pos < 0) return null
            values[idx] = pos
        }
        val out = ByteArray((input.length * 3) / 4)
        var outIdx = 0
        var i = 0
        while (i < values.size) {
            val c0 = values[i]
            val c1 = values[i + 1]
            out[outIdx++] = ((c0 shl 2) or (c1 ushr 4)).toByte()
            if (i + 2 < values.size) {
                val c2 = values[i + 2]
                out[outIdx++] = (((c1 and 0x0F) shl 4) or (c2 ushr 2)).toByte()
                if (i + 3 < values.size) {
                    val c3 = values[i + 3]
                    out[outIdx++] = (((c2 and 0x03) shl 6) or c3).toByte()
                }
            }
            i += 4
        }
        return out.copyOf(outIdx)
    }
}
