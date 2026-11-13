package com.alenia.mnemis.model

import android.net.Uri

/**
 * Represents a folder/album grouping local videos from MediaStore.
 */
data class VideoAlbum(
    val name: String,
    val coverUri: Uri,
    val videos: List<VideoItem>
) {
    val count: Int get() = videos.size
}

