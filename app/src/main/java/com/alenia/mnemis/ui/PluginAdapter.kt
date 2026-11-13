package com.alenia.mnemis.ui

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.alenia.mnemis.databinding.ItemPluginBinding
import com.alenia.mnemis.model.PluginItem

class PluginAdapter(
    private val onToggleEnabled: (PluginItem, Boolean) -> Unit,
    private val onInstallClick: (PluginItem) -> Unit
) : ListAdapter<PluginItem, PluginAdapter.PluginViewHolder>(PluginDiffCallback()) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): PluginViewHolder {
        val binding = ItemPluginBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return PluginViewHolder(binding)
    }

    override fun onBindViewHolder(holder: PluginViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    inner class PluginViewHolder(private val binding: ItemPluginBinding) :
        RecyclerView.ViewHolder(binding.root) {

        fun bind(item: PluginItem) {
            binding.tvPluginName.text = item.name
            binding.tvPluginVersion.text = item.version
            binding.tvPluginDesc.text = item.description

            if (item.isInstalled) {
                binding.btnInstallPlugin.visibility = View.GONE
                binding.switchPlugin.visibility = View.VISIBLE
                binding.switchPlugin.setOnCheckedChangeListener(null)
                binding.switchPlugin.isChecked = item.isEnabled
                binding.switchPlugin.setOnCheckedChangeListener { _, isChecked ->
                    onToggleEnabled(item, isChecked)
                }
            } else {
                binding.switchPlugin.visibility = View.GONE
                binding.btnInstallPlugin.visibility = View.VISIBLE
                binding.btnInstallPlugin.setOnClickListener {
                    onInstallClick(item)
                }
            }
        }
    }

    private class PluginDiffCallback : DiffUtil.ItemCallback<PluginItem>() {
        override fun areItemsTheSame(oldItem: PluginItem, newItem: PluginItem): Boolean {
            return oldItem.id == newItem.id
        }

        override fun areContentsTheSame(oldItem: PluginItem, newItem: PluginItem): Boolean {
            return oldItem == newItem
        }
    }
}

