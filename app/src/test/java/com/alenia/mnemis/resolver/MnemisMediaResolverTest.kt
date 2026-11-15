package com.alenia.mnemis.resolver

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertTrue
import org.junit.Test

/**
 * Tests the YouTube video-ID extraction that the PiP handover depends on:
 * [MnemisMediaResolver.extractYouTubeVideoId] decides whether a page is a
 * YouTube video (hand over to the native player) or another platform
 * (plain WebView PiP).
 */
class MnemisMediaResolverTest {

    @Test
    fun extractYouTubeVideoId_watchUrls() {
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/watch?v=dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/watch?v=dQw4w9WgXcQ&t=42s")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/watch?list=PL1234567890&v=dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://m.youtube.com/watch?v=dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://music.youtube.com/watch?v=dQw4w9WgXcQ")
        )
    }

    @Test
    fun extractYouTubeVideoId_shortUrlsAndPaths() {
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://youtu.be/dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://youtu.be/dQw4w9WgXcQ?t=10")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/embed/dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/shorts/dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/live/dQw4w9WgXcQ")
        )
        assertEquals(
            "dQw4w9WgXcQ",
            MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube-nocookie.com/embed/dQw4w9WgXcQ")
        )
    }

    @Test
    fun extractYouTubeVideoId_nonVideoUrls() {
        assertNull(MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/"))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/feed/subscriptions"))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId("https://www.youtube.com/results?search_query=test"))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId("https://vix.com/es/videos/123"))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId("https://www.twitch.tv/somechannel"))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId(null))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId(""))
        assertNull(MnemisMediaResolver.extractYouTubeVideoId("   "))
    }

    @Test
    fun isYouTubeVideoUrl_matchesOnlyVideos() {
        assertTrue(MnemisMediaResolver.isYouTubeVideoUrl("https://www.youtube.com/watch?v=dQw4w9WgXcQ"))
        assertTrue(MnemisMediaResolver.isYouTubeVideoUrl("https://youtu.be/dQw4w9WgXcQ"))
        assertFalse(MnemisMediaResolver.isYouTubeVideoUrl("https://www.youtube.com/"))
        assertFalse(MnemisMediaResolver.isYouTubeVideoUrl("https://www.twitch.tv/somechannel"))
    }
}
