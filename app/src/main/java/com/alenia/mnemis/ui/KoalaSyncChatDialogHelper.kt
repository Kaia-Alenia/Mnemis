package com.alenia.mnemis.ui

import android.content.Context
import android.view.LayoutInflater
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.LifecycleCoroutineScope
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.DialogChatBinding
import com.alenia.mnemis.sync.KoalaSyncClient
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.launch
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

object KoalaSyncChatDialogHelper {

    fun show(
        context: Context,
        lifecycleScope: LifecycleCoroutineScope,
        koalaSyncClient: KoalaSyncClient
    ) {
        val chatBinding = DialogChatBinding.inflate(LayoutInflater.from(context))
        val dialog = AlertDialog.Builder(context)
            .setView(chatBinding.root)
            .setCancelable(true)
            .create()

        val activeSecret = koalaSyncClient.getActiveChatSecret()
        if (!activeSecret.isNullOrBlank()) {
            chatBinding.tvChatSecretInfo.visibility = View.VISIBLE
            chatBinding.tvChatSecretInfo.text = context.getString(R.string.chat_secret_badge, activeSecret)
        } else {
            chatBinding.tvChatSecretInfo.visibility = View.GONE
        }

        val timeFormat = SimpleDateFormat("HH:mm", Locale.getDefault())
        val job = lifecycleScope.launch {
            koalaSyncClient.chatMessages.collectLatest { messages ->
                if (messages.isEmpty()) {
                    chatBinding.tvChatHistory.text = context.getString(R.string.chat_empty)
                } else {
                    val formatted = messages.joinToString("\n\n") { msg ->
                        val time = timeFormat.format(Date(msg.timestampMs))
                        "[${time}] ${msg.senderName}: ${msg.text}"
                    }
                    chatBinding.tvChatHistory.text = formatted
                    chatBinding.scrollChatMessages.post {
                        chatBinding.scrollChatMessages.fullScroll(View.FOCUS_DOWN)
                    }
                }
            }
        }

        val sendText: (String) -> Unit = { text ->
            val trimmed = text.trim()
            if (trimmed.isNotEmpty()) {
                val sent = koalaSyncClient.sendChatMessage(trimmed)
                if (!sent) {
                    Toast.makeText(context, R.string.chat_send_failed, Toast.LENGTH_SHORT).show()
                }
            }
        }

        chatBinding.btnSendChat.setOnClickListener {
            val text = chatBinding.etChatInput.text?.toString().orEmpty()
            if (text.isNotBlank()) {
                sendText(text)
                chatBinding.etChatInput.setText("")
            }
        }

        chatBinding.btnReactFire.setOnClickListener { sendText("🔥") }
        chatBinding.btnReactLaugh.setOnClickListener { sendText("😂") }
        chatBinding.btnReactWow.setOnClickListener { sendText("😮") }
        chatBinding.btnReactClap.setOnClickListener { sendText("👏") }
        chatBinding.btnReactHeart.setOnClickListener { sendText("❤️") }

        dialog.setOnDismissListener {
            job.cancel()
        }
        dialog.show()
    }
}

