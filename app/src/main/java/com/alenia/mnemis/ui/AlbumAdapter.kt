package com.alenia.mnemis.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.alenia.mnemis.R
import com.alenia.mnemis.databinding.ItemAlbumBinding
import com.alenia.mnemis.model.VideoAlbum
import com.bumptech.glide.Glide

class AlbumAdapter(
    private val onAlbumClick: (VideoAlbum) -> Unit
) : ListAdapter<VideoAlbum, AlbumAdapter.AlbumViewHolder>(AlbumDiffCallback()) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): AlbumViewHolder {
        val binding = ItemAlbumBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return AlbumViewHolder(binding)
    }

    override fun onBindViewHolder(holder: AlbumViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    inner class AlbumViewHolder(private val binding: ItemAlbumBinding) :
        RecyclerView.ViewHolder(binding.root) {

        fun bind(album: VideoAlbum) {
            binding.tvAlbumName.text = album.name
            binding.tvAlbumCount.text = binding.root.context.resources.getQuantityString(
                R.plurals.video_count,
                album.count,
                album.count
            )

            Glide.with(binding.ivAlbumCover.context)
                .load(album.coverUri)
                .centerCrop()
                .into(binding.ivAlbumCover)

            binding.root.setOnClickListener {
                onAlbumClick(album)
            }
        }
    }

    private class AlbumDiffCallback : DiffUtil.ItemCallback<VideoAlbum>() {
        override fun areItemsTheSame(oldItem: VideoAlbum, newItem: VideoAlbum): Boolean {
            return oldItem.name == newItem.name
        }

        override fun areContentsTheSame(oldItem: VideoAlbum, newItem: VideoAlbum): Boolean {
            return oldItem == newItem
        }
    }
}

