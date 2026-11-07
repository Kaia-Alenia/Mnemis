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
        "(?:youtu\\.be\\/|youtube\\.com\\/(?:watch\\?(?:.*&)?v=|(?:embed|v|shorts)\\/))([a-zA-Z0-9_-]{11})"
    );
    private static final Pattern GDRIVE_PATTERN = Pattern.compile(
        "drive\\.google\\.com\\/file\\/d\\/([a-zA-Z0-9_-]+)"
    );

    /**
     * Resuelve cualquier URL (YouTube, Google Drive, Dropbox, etc.) a una URL directa de streaming multimedia.
     */
    public static String resolve(String rawUrl) {
        if (rawUrl == null || rawUrl.trim().isEmpty()) {
            return rawUrl;
        }

        final String url = rawUrl.trim();

        // 1. Deteccion de Google Drive
        Matcher gdriveMatcher = GDRIVE_PATTERN.matcher(url);
        if (gdriveMatcher.find()) {
            String fileId = gdriveMatcher.group(1);
            String directUrl = "https://drive.google.com/uc?export=download&id=" + fileId;
            Log.i(TAG, "Mnemis resolved Google Drive link: " + directUrl);
            return directUrl;
        }

        // 2. Deteccion de Dropbox
        if (url.contains("dropbox.com") && url.contains("dl=0")) {
            return url.replace("dl=0", "dl=1");
        }

        // 3. Deteccion de YouTube
        Matcher ytMatcher = YT_PATTERN.matcher(url);
        if (ytMatcher.find()) {
            final String videoId = ytMatcher.group(1);
            Log.i(TAG, "Mnemis resolving YouTube video ID: " + videoId);

            Future<String> future = EXECUTOR.submit(new Callable<String>() {
                @Override
                public String call() {
                    return resolveYouTubeVideoId(videoId);
                }
            });

            try {
                // Timeout de 5 segundos para no congelar si la red es lenta
                String resolved = future.get(5000, TimeUnit.MILLISECONDS);
                if (resolved != null && !resolved.isEmpty()) {
                    Log.i(TAG, "Mnemis successfully resolved YouTube direct stream URL");
                    return resolved;
                }
            } catch (Exception e) {
                Log.w(TAG, "Mnemis failed to resolve YouTube link, falling back: " + e.getMessage());
            }
        }

        return url;
    }

    private static String resolveYouTubeVideoId(String videoId) {
        HttpURLConnection conn = null;
        try {
            URL url = new URL("https://www.youtube.com/youtubei/v1/player");
            conn = (HttpURLConnection) url.openConnection();
            conn.setRequestMethod("POST");
            conn.setRequestProperty("Content-Type", "application/json");
            conn.setRequestProperty("User-Agent", "com.google.android.youtube/1.60.19 (Linux; U; Android 12)");
            conn.setConnectTimeout(4000);
            conn.setReadTimeout(4000);
            conn.setDoOutput(true);

            String requestBody = "{\"context\":{\"client\":{\"clientName\":\"ANDROID_VR\",\"clientVersion\":\"1.60.19\"}},\"videoId\":\"" + videoId + "\"}";
            
            try (OutputStream os = conn.getOutputStream()) {
                os.write(requestBody.getBytes(StandardCharsets.UTF_8));
                os.flush();
            }

            int responseCode = conn.getResponseCode();
            if (responseCode != 200) {
                Log.w(TAG, "YouTube API response code: " + responseCode);
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
                return null;
            }

            // 1. Priorizar formato con audio y video combinados (formats)
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

            // 2. Si no, buscar en adaptiveFormats
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

            // 3. Fallback a HLS manifest si existe
            String hls = streamingData.optString("hlsManifestUrl", null);
            if (hls != null && !hls.isEmpty()) {
                return hls;
            }

        } catch (Exception e) {
            Log.e(TAG, "Error in resolveYouTubeVideoId", e);
        } finally {
            if (conn != null) {
                conn.disconnect();
            }
        }
        return null;
    }
}

