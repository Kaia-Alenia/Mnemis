package com.alenia.mnemis.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.alenia.mnemis.databinding.ItemVideoBinding
import com.alenia.mnemis.model.VideoItem
import com.bumptech.glide.Glide
import com.bumptech.glide.load.engine.DiskCacheStrategy

class VideoAdapter(
    private val onVideoClick: (VideoItem) -> Unit
) : RecyclerView.Adapter<VideoAdapter.VideoViewHolder>() {

    private val videoList = mutableListOf<VideoItem>()

    fun submitList(videos: List<VideoItem>) {
        videoList.clear()
        videoList.addAll(videos)
        notifyDataSetChanged()
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): VideoViewHolder {
        val binding = ItemVideoBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return VideoViewHolder(binding)
    }

    override fun onBindViewHolder(holder: VideoViewHolder, position: Int) {
        holder.bind(videoList[position])
    }

    override fun getItemCount(): Int = videoList.size

    inner class VideoViewHolder(private val binding: ItemVideoBinding) : RecyclerView.ViewHolder(binding.root) {
        fun bind(item: VideoItem) {
            binding.tvTitle.text = item.title
            binding.tvDuration.text = item.getFormattedDuration()

            val metaParts = mutableListOf<String>()
            if (item.resolution.isNotEmpty()) metaParts.add(item.resolution)
            val sizeStr = item.getFormattedSize()
            if (sizeStr.isNotEmpty()) metaParts.add(sizeStr)
            binding.tvMeta.text = metaParts.joinToString(" • ")

            Glide.with(binding.ivThumbnail.context)
                .load(item.uri)
                .diskCacheStrategy(DiskCacheStrategy.ALL)
                .centerCrop()
                .into(binding.ivThumbnail)

            binding.root.setOnClickListener {
                onVideoClick(item)
            }
        }
    }
}

