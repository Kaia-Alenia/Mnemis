package com.alenia.mnemis.sync

import android.webkit.JavascriptInterface
import android.webkit.WebView
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.withTimeoutOrNull
import org.json.JSONObject
import kotlin.coroutines.resume

/**
 * Bidirectional JS bridge between an Android WebView's HTML5 <video> element and KoalaSyncClient.
 */
class KoalaSyncWebBridge(
    private val webView: WebView,
    private val syncClient: KoalaSyncClient,
    private val onRunOnUiThread: (() -> Unit) -> Unit
) {

    @Volatile
    private var lastSnapshot = KoalaSyncClient.PlaybackSnapshot(
        isPlaying = false,
        currentTimeSeconds = 0.0,
        mediaTitle = null
    )

    @JavascriptInterface
    fun onPlay(currentTimeSeconds: Double) {
        if (!currentTimeSeconds.isNaN()) {
            syncClient.sendPlay(currentTimeSeconds)
        }
    }

    @JavascriptInterface
    fun onPause(currentTimeSeconds: Double) {
        if (!currentTimeSeconds.isNaN()) {
            syncClient.sendPause(currentTimeSeconds)
        }
    }

    @JavascriptInterface
    fun onSeek(targetTimeSeconds: Double) {
        if (!targetTimeSeconds.isNaN()) {
            syncClient.sendSeek(targetTimeSeconds)
        }
    }

    @JavascriptInterface
    fun onSnapshot(isPlaying: Boolean, currentTimeSeconds: Double, title: String?) {
        lastSnapshot = KoalaSyncClient.PlaybackSnapshot(
            isPlaying = isPlaying,
            currentTimeSeconds = if (currentTimeSeconds.isNaN()) 0.0 else currentTimeSeconds,
            mediaTitle = title?.takeIf { it.isNotBlank() }
        )
    }

    suspend fun getLatestSnapshot(): KoalaSyncClient.PlaybackSnapshot {
        val evaluated = withTimeoutOrNull(500L) {
            suspendCancellableCoroutine { cont ->
                onRunOnUiThread {
                    webView.evaluateJavascript(
                        "window.__mnemisKoalaSync ? window.__mnemisKoalaSync.getSnapshotJson() : null;"
                    ) { raw ->
                        if (cont.isActive) {
                            cont.resume(parseSnapshotJson(raw))
                        }
                    }
                }
            }
        }
        if (evaluated != null) {
            lastSnapshot = evaluated
        }
        return lastSnapshot
    }

    private fun parseSnapshotJson(raw: String?): KoalaSyncClient.PlaybackSnapshot? {
        if (raw.isNullOrBlank() || raw == "null") return null
        return try {
            // evaluateJavascript returns a JSON-encoded string literal.
            val unquoted = if (raw.startsWith("\"")) {
                JSONObject("{\"v\":$raw}").getString("v")
            } else {
                raw
            }
            val obj = JSONObject(unquoted)
            KoalaSyncClient.PlaybackSnapshot(
                isPlaying = obj.optBoolean("isPlaying", false),
                currentTimeSeconds = obj.optDouble("currentTime", 0.0).let {
                    if (it.isNaN()) 0.0 else it
                },
                mediaTitle = obj.optString("title", "").takeIf { it.isNotBlank() }
            )
        } catch (_: Exception) {
            null
        }
    }

    fun injectEarlyAdBlock() {
        webView.evaluateJavascript(EARLY_ADBLOCK_JS, null)
    }

    fun injectScript() {
        webView.evaluateJavascript(INJECTED_JS, null)
    }

    fun applyRemotePlay(currentTimeSeconds: Double?) {
        val timeArg = currentTimeSeconds?.toString() ?: "null"
        webView.evaluateJavascript("window.__mnemisKoalaSync && window.__mnemisKoalaSync.applyPlay($timeArg);", null)
    }

    fun applyRemotePause(currentTimeSeconds: Double?) {
        val timeArg = currentTimeSeconds?.toString() ?: "null"
        webView.evaluateJavascript("window.__mnemisKoalaSync && window.__mnemisKoalaSync.applyPause($timeArg);", null)
    }

    fun applyRemoteSeek(targetTimeSeconds: Double) {
        webView.evaluateJavascript(
            "window.__mnemisKoalaSync && window.__mnemisKoalaSync.applySeek($targetTimeSeconds);",
            null
        )
    }

    fun correctDriftIfNeeded(expectedTimeSeconds: Double) {
        webView.evaluateJavascript(
            "window.__mnemisKoalaSync && window.__mnemisKoalaSync.correctDrift($expectedTimeSeconds);",
            null
        )
    }

    companion object {
        const val JS_INTERFACE_NAME = "MnemisKoalaBridge"

        val EARLY_ADBLOCK_JS: String = """
            (function() {
                if (window.__mnemisEarlyAdBlock) return;
                window.__mnemisEarlyAdBlock = true;

                function stripAds(obj) {
                    if (obj && typeof obj === 'object') {
                        if ('adPlacements' in obj) delete obj.adPlacements;
                        if ('playerAds' in obj) delete obj.playerAds;
                        if ('adSlots' in obj) delete obj.adSlots;
                    }
                    return obj;
                }

                if (window.ytInitialPlayerResponse) {
                    stripAds(window.ytInitialPlayerResponse);
                }

                const origParse = JSON.parse;
                JSON.parse = function(text, reviver) {
                    const res = origParse.call(this, text, reviver);
                    return stripAds(res);
                };
            })();
        """.trimIndent()

        val INJECTED_JS: String = """
            (function() {
                if (window.__mnemisKoalaSync) {
                    window.__mnemisKoalaSync.scan();
                    return;
                }

                const SUPPRESS_MS = 600;
                const DRIFT_THRESHOLD_SEC = 0.4;
                const SOFT_DRIFT_THRESHOLD_SEC = 0.5;
                const HARD_DRIFT_THRESHOLD_SEC = 2.5;
                let suppressUntil = 0;
                let boundVideo = null;
                let mutedByAdSkip = false;
                let scanTimer = null;
                let rateResetTimer = null;
                const boundVideos = new WeakSet();

                function isSuppressed() {
                    return Date.now() < suppressUntil;
                }

                function setSuppress() {
                    suppressUntil = Date.now() + SUPPRESS_MS;
                }

                function isAdShowing() {
                    const ytPlayer = document.getElementById('movie_player');
                    if (ytPlayer && (ytPlayer.classList.contains('ad-showing') || ytPlayer.classList.contains('ad-interrupting'))) {
                        return true;
                    }
                    const overlay = document.querySelector('.ytp-ad-player-overlay, .ytp-ad-skip-button, .ytp-skip-ad-button');
                    return !!(overlay && overlay.offsetParent !== null);
                }

                function injectAdHideCss() {
                    if (document.getElementById('mnemis-adblock-css')) return;
                    const style = document.createElement('style');
                    style.id = 'mnemis-adblock-css';
                    style.textContent = `
                        ytd-ad-slot-renderer,
                        ytm-promoted-sparkles-web-renderer,
                        ytm-companion-slot-renderer,
                        #player-ads,
                        .ytp-ad-overlay-container,
                        .ytp-ad-text-overlay {
                            display: none !important;
                        }
                    `;
                    (document.head || document.documentElement).appendChild(style);
                }

                function trySkipAd() {
                    const v = document.querySelector('video');
                    if (!isAdShowing()) {
                        if (v) {
                            if (v.playbackRate > 2.0) v.playbackRate = 1.0;
                            if (mutedByAdSkip) {
                                v.muted = false;
                                mutedByAdSkip = false;
                            }
                        }
                        return;
                    }
                    const skipBtn = document.querySelector('.ytp-ad-skip-button, .ytp-skip-ad-button, .ytp-ad-skip-button-modern, .ytp-ad-skip-button-container button, button[class*="skip"]');
                    if (skipBtn) {
                        skipBtn.click();
                    }
                    if (v) {
                        if (!v.muted) {
                            v.muted = true;
                            mutedByAdSkip = true;
                        }
                        if (Number.isFinite(v.duration) && v.duration > 0 && v.currentTime < v.duration - 0.5) {
                            v.currentTime = v.duration;
                        } else {
                            v.playbackRate = 16.0;
                        }
                    }
                }

                function seekVideoElement(v, targetTime) {
                    const ytPlayer = document.getElementById('movie_player');
                    if (ytPlayer && typeof ytPlayer.seekTo === 'function') {
                        try {
                            ytPlayer.seekTo(targetTime, true);
                            return;
                        } catch (_) {}
                    }
                    v.currentTime = targetTime;
                }

                function playVideoElement(v) {
                    const ytPlayer = document.getElementById('movie_player');
                    if (ytPlayer && typeof ytPlayer.playVideo === 'function') {
                        try {
                            ytPlayer.playVideo();
                        } catch (_) {}
                    }
                    const attemptPlay = () => {
                        const p = v.play();
                        if (p && typeof p.catch === 'function') {
                            p.catch(() => {});
                        }
                    };
                    attemptPlay();
                    if (v.readyState < 3) {
                        v.addEventListener('canplay', () => {
                            if (v.paused) attemptPlay();
                        }, { once: true });
                    }
                }

                function pauseVideoElement(v) {
                    const ytPlayer = document.getElementById('movie_player');
                    if (ytPlayer && typeof ytPlayer.pauseVideo === 'function') {
                        try {
                            ytPlayer.pauseVideo();
                        } catch (_) {}
                    }
                    v.pause();
                }

                function collectVideoCandidates() {
                    const direct = Array.from(document.querySelectorAll('video'));
                    const deep = typeof window.__mnemisCollectDeepVideos === 'function'
                        ? window.__mnemisCollectDeepVideos()
                        : [];
                    const videos = Array.from(new Set(direct.concat(deep)));
                    return videos.filter(v => {
                        const rect = v.getBoundingClientRect();
                        return rect.width > 0 && rect.height > 0;
                    }).sort((a, b) => {
                        const aPlaying = (!a.paused && !a.ended) ? 1 : 0;
                        const bPlaying = (!b.paused && !b.ended) ? 1 : 0;
                        if (aPlaying !== bPlaying) return bPlaying - aPlaying;
                        const aArea = a.clientWidth * a.clientHeight;
                        const bArea = b.clientWidth * b.clientHeight;
                        return bArea - aArea;
                    });
                }

                function pickPrimaryVideo() {
                    const ytMain = document.querySelector('#movie_player video, video.html5-main-video');
                    if (ytMain) return ytMain;
                    const candidates = collectVideoCandidates();
                    if (candidates.length > 0) return candidates[0];
                    const deep = typeof window.__mnemisCollectDeepVideos === 'function'
                        ? window.__mnemisCollectDeepVideos()
                        : [];
                    return document.querySelector('video') || (deep.length > 0 ? deep[0] : null);
                }

                function getActiveVideo() {
                    if (boundVideo && boundVideo.isConnected) return boundVideo;
                    return pickPrimaryVideo();
                }

                function buildSnapshotObject() {
                    const v = getActiveVideo();
                    if (!v || isAdShowing()) return null;
                    const cleanTitle = (document.title || '').replace(/\s*-\s*YouTube$/i, '').trim();
                    return {
                        isPlaying: !v.paused && !v.ended,
                        currentTime: Number.isFinite(v.currentTime) ? v.currentTime : 0,
                        title: cleanTitle
                    };
                }

                function getSnapshotJson() {
                    const snap = buildSnapshotObject();
                    return snap ? JSON.stringify(snap) : null;
                }

                function reportSnapshot() {
                    const snap = buildSnapshotObject();
                    if (!snap || !window.$JS_INTERFACE_NAME) return;
                    window.$JS_INTERFACE_NAME.onSnapshot(snap.isPlaying, snap.currentTime, snap.title);
                }

                function bindVideo(video) {
                    if (!video) return;
                    boundVideo = video;
                    if (boundVideos.has(video)) {
                        reportSnapshot();
                        return;
                    }
                    boundVideos.add(video);

                    video.addEventListener('play', () => {
                        if (isSuppressed() || isAdShowing()) return;
                        window.$JS_INTERFACE_NAME && window.$JS_INTERFACE_NAME.onPlay(video.currentTime || 0);
                    });

                    video.addEventListener('pause', () => {
                        if (isSuppressed() || isAdShowing()) return;
                        window.$JS_INTERFACE_NAME && window.$JS_INTERFACE_NAME.onPause(video.currentTime || 0);
                    });

                    video.addEventListener('seeked', () => {
                        if (isSuppressed() || isAdShowing()) return;
                        window.$JS_INTERFACE_NAME && window.$JS_INTERFACE_NAME.onSeek(video.currentTime || 0);
                    });

                    reportSnapshot();
                }

                function scan() {
                    injectAdHideCss();
                    trySkipAd();
                    if (boundVideo && boundVideo.isConnected) return;
                    const candidate = pickPrimaryVideo();
                    if (candidate) {
                        bindVideo(candidate);
                    }
                }

                function scheduleScan() {
                    if (scanTimer) return;
                    scanTimer = setTimeout(() => {
                        scanTimer = null;
                        scan();
                    }, 500);
                }

                const observer = new MutationObserver(() => scheduleScan());
                if (document.documentElement) {
                    observer.observe(document.documentElement, { childList: true, subtree: true });
                }
                setInterval(() => trySkipAd(), 800);
                window.addEventListener('yt-navigate-finish', () => setTimeout(scan, 300));

                window.__mnemisKoalaSync = {
                    scan,
                    reportSnapshot,
                    getSnapshotJson,
                    applyPlay(targetTime) {
                        const v = getActiveVideo();
                        if (!v) return;
                        setSuppress();
                        if (typeof targetTime === 'number' && Number.isFinite(targetTime)) {
                            if (Math.abs((v.currentTime || 0) - targetTime) > DRIFT_THRESHOLD_SEC) {
                                seekVideoElement(v, targetTime);
                            }
                        }
                        playVideoElement(v);
                    },
                    applyPause(targetTime) {
                        const v = getActiveVideo();
                        if (!v) return;
                        setSuppress();
                        if (typeof targetTime === 'number' && Number.isFinite(targetTime)) {
                            if (Math.abs((v.currentTime || 0) - targetTime) > DRIFT_THRESHOLD_SEC) {
                                seekVideoElement(v, targetTime);
                            }
                        }
                        pauseVideoElement(v);
                    },
                    applySeek(targetTime) {
                        const v = getActiveVideo();
                        if (!v || typeof targetTime !== 'number' || !Number.isFinite(targetTime)) return;
                        setSuppress();
                        seekVideoElement(v, targetTime);
                    },
                    correctDrift(expectedTime) {
                        const v = getActiveVideo();
                        if (!v || v.paused || v.ended || isAdShowing() || typeof expectedTime !== 'number' || !Number.isFinite(expectedTime)) return;
                        const diff = expectedTime - (v.currentTime || 0);
                        const absDiff = Math.abs(diff);
                        if (absDiff > HARD_DRIFT_THRESHOLD_SEC) {
                            setSuppress();
                            v.playbackRate = 1.0;
                            seekVideoElement(v, expectedTime);
                        } else if (absDiff > SOFT_DRIFT_THRESHOLD_SEC) {
                            v.playbackRate = diff > 0 ? 1.15 : 0.85;
                            if (rateResetTimer) clearTimeout(rateResetTimer);
                            rateResetTimer = setTimeout(() => {
                                v.playbackRate = 1.0;
                            }, 3500);
                        } else if (v.playbackRate !== 1.0) {
                            v.playbackRate = 1.0;
                        }
                    }
                };

                scan();
            })();
        """.trimIndent()
    }
}
