package com.alenia.mnemis.resolver;

import android.util.Log;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class MnemisMediaResolver {
    private static final String TAG = "MnemisResolver";
    private static final ExecutorService EXECUTOR = Executors.newCachedThreadPool();

    private static final Pattern YT_PATTERN = Pattern.compile(
        "(?:youtu\\.be\\/|(?:www\\.|m\\.|music\\.)?youtube(?:-nocookie)?\\.com\\/(?:watch\\?(?:.*&)?v=|(?:embed|v|shorts|live)\\/))([a-zA-Z0-9_-]{11})"
    );
    private static final Pattern GDRIVE_PATTERN = Pattern.compile(
        "drive\\.google\\.com\\/file\\/d\\/([a-zA-Z0-9_-]+)"
    );

    /**
     * Sanitiza el tipo de contenedor multimedia (HLS vs Progresivo MP4) para ExoPlayer.
     */
    public static String sanitizeType(android.net.Uri uri, String type) {
        if (uri == null) {
            return type;
        }
        String s = uri.toString().toLowerCase();
        if (s.contains("googlevideo.com") || s.contains("drive.google.com") || s.contains(".mp4") || s.contains("mime=video%2fmp4") || s.contains("mime=video/mp4")) {
            Log.e(TAG, "Mnemis detected progressive MP4 stream. Disabling HLS container mode.");
            return null;
        }
        if (s.contains(".m3u8") || s.contains("m3u8") || s.contains("manifest/hls")) {
            return "hls";
        }
        return type;
    }

    /**
     * Resuelve cualquier URL (YouTube, Google Drive, Dropbox, etc.) a una URL directa de streaming multimedia.
     */
    public static String resolve(String rawUrl) {
        if (rawUrl == null || rawUrl.trim().isEmpty()) {
            return rawUrl;
        }

        final String url = rawUrl.trim();
        Log.e(TAG, "Mnemis resolving input URL: " + url);

        // 1. Deteccion de Google Drive
        Matcher gdriveMatcher = GDRIVE_PATTERN.matcher(url);
        if (gdriveMatcher.find()) {
            String fileId = gdriveMatcher.group(1);
            String directUrl = "https://drive.google.com/uc?export=download&id=" + fileId;
            Log.e(TAG, "Mnemis resolved Google Drive direct stream: " + directUrl);
            return directUrl;
        }

        // 2. Deteccion de Dropbox
        if (url.contains("dropbox.com") && url.contains("dl=0")) {
            String directUrl = url.replace("dl=0", "dl=1");
            Log.e(TAG, "Mnemis resolved Dropbox direct stream: " + directUrl);
            return directUrl;
        }

        // 3. Deteccion de YouTube
        Matcher ytMatcher = YT_PATTERN.matcher(url);
        if (ytMatcher.find()) {
            final String videoId = ytMatcher.group(1);
            Log.e(TAG, "Mnemis matched YouTube video ID: " + videoId);

            Future<String> future = EXECUTOR.submit(new Callable<String>() {
                @Override
                public String call() {
                    return resolveYouTubeVideo(videoId);
                }
            });

            try {
                String resolved = future.get(6000, TimeUnit.MILLISECONDS);
                if (resolved != null && !resolved.isEmpty()) {
                    Log.e(TAG, "Mnemis successfully resolved YouTube stream URL: " + resolved.substring(0, Math.min(60, resolved.length())) + "...");
                    return resolved;
                }
            } catch (Exception e) {
                Log.e(TAG, "Mnemis failed to resolve YouTube link in time: " + e.getMessage(), e);
            }
        }

        return url;
    }

    private static String resolveYouTubeVideo(String videoId) {
        // Metodo 1: YouTube InnerTube API usando cliente ANDROID_VR (proporciona MP4 directo con audio+video sin cifrar)
        String stream = queryInnerTube(videoId, "ANDROID_VR", "1.60.19", "com.google.android.youtube/1.60.19 (Linux; U; Android 12)");
        if (stream != null) {
            return stream;
        }

        // Metodo 2: YouTube InnerTube API usando cliente IOS
        stream = queryInnerTube(videoId, "IOS", "19.29.1", "com.google.ios.youtube/19.29.1 (iPhone14,3; U; CPU iOS 16_6 like Mac OS X)");
        if (stream != null) {
            return stream;
        }

        // Metodo 3: Invidious API como fallback publico
        return queryInvidiousFallback(videoId);
    }

    private static String queryInnerTube(String videoId, String clientName, String clientVersion, String userAgent) {
        HttpURLConnection conn = null;
        try {
            URL url = new URL("https://www.youtube.com/youtubei/v1/player");
            conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setRequestProperty("User-Agent", userAgent);
            conn.setConnectTimeout(4500);
            conn.setReadTimeout(4500);
            conn.setDoOutput(true);

            String requestBody = "{\"context\":{\"client\":{\"clientName\":\"" + clientName + "\",\"clientVersion\":\"" + clientVersion + "\"}},\"videoId\":\"" + videoId + "\"}";

            try (OutputStream os = conn.getOutputStream()) {
                os.write(requestBody.getBytes(StandardCharsets.UTF_8));
                os.flush();
            }

            int code = conn.getResponseCode();
            if (code != 200) {
                Log.e(TAG, "InnerTube [" + clientName + "] HTTP code: " + code);
                return null;
            }

            StringBuilder sb = new StringBuilder();
            try (InputStream is = conn.getInputStream();
                 BufferedReader reader = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line);
                }
            }

            JSONObject json = new JSONObject(sb.toString());
            JSONObject streamingData = json.optJSONObject("streamingData");
            if (streamingData == null) {
                Log.e(TAG, "InnerTube [" + clientName + "] streamingData is null");
                return null;
            }

            // Priorizar formatos multiplexados (video + audio en un solo stream MP4)
            JSONArray formats = streamingData.optJSONArray("formats");
            if (formats != null && formats.length() > 0) {
                for (int i = 0; i < formats.length(); i++) {
                    JSONObject fmt = formats.getJSONObject(i);
                    String directUrl = fmt.optString("url", null);
                    if (directUrl != null && !directUrl.isEmpty()) {
                        return directUrl;
                    }
                }
            }

            // Fallback: HLS manifest
            String hls = streamingData.optString("hlsManifestUrl", null);
            if (hls != null && !hls.isEmpty()) {
                return hls;
            }

            // Fallback: adaptiveFormats
            JSONArray adaptive = streamingData.optJSONArray("adaptiveFormats");
            if (adaptive != null && adaptive.length() > 0) {
                for (int i = 0; i < adaptive.length(); i++) {
                    JSONObject fmt = adaptive.getJSONObject(i);
                    String mime = fmt.optString("mimeType", "");
                    if (mime.contains("video/mp4")) {
                        String directUrl = fmt.optString("url", null);
                        if (directUrl != null && !directUrl.isEmpty()) {
                            return directUrl;
                        }
                    }
                }
            }

        } catch (Exception e) {
            Log.e(TAG, "InnerTube [" + clientName + "] error: " + e.getMessage());
        } finally {
            if (conn != null) {
                conn.disconnect();
            }
        }
        return null;
    }

    private static String queryInvidiousFallback(String videoId) {
        String[] instances = new String[]{
            "https://inv.tux.pizza",
            "https://invidious.nerdvpn.de",
            "https://vid.puffyan.us"
        };

        for (String instance : instances) {
            HttpURLConnection conn = null;
            try {
                URL url = new URL(instance + "/api/v1/videos/" + videoId);
                conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("GET");
                conn.setConnectTimeout(3000);
                conn.setReadTimeout(3000);
                conn.setRequestProperty("User-Agent", "Mnemis/2.0");

                if (conn.getResponseCode() == 200) {
                    StringBuilder sb = new StringBuilder();
                    try (InputStream is = conn.getInputStream();
                         BufferedReader reader = new BufferedReader(new InputStreamReader(is, StandardCharsets.UTF_8))) {
                        String line;
                        while ((line = reader.readLine()) != null) {
                            sb.append(line);
                        }
                    }

                    JSONObject json = new JSONObject(sb.toString());
                    JSONArray formatStreams = json.optJSONArray("formatStreams");
                    if (formatStreams != null && formatStreams.length() > 0) {
                        for (int i = formatStreams.length() - 1; i >= 0; i--) {
                            JSONObject fs = formatStreams.getJSONObject(i);
                            String directUrl = fs.optString("url", null);
                            if (directUrl != null && !directUrl.isEmpty()) {
                                Log.e(TAG, "Invidious fallback matched direct stream URL from " + instance);
                                return directUrl;
                            }
                        }
                    }
                }
            } catch (Exception e) {
                // Continuar al siguiente mirror
            } finally {
                if (conn != null) {
                    conn.disconnect();
                }
            }
        }
        return null;
    }
}
