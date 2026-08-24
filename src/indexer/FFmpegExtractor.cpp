#include "FFmpegExtractor.hpp"

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/log.h>
}

#include <iostream>
#include <memory>
#include <mutex>

namespace mnemis::indexer {

#include <charconv>

namespace {
    std::once_flag ffmpeg_init_flag;

    void ffmpegLogCallback(void* ptr, int level, const char* fmt, va_list vl) {
        if (level > av_log_get_level()) return;
        char line[1024];
        static int print_prefix = 1;
        av_log_format_line2(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
        std::cerr << "[FFmpeg] " << line;
    }

    void initFFmpeg() {
        std::call_once(ffmpeg_init_flag, []() {
            av_log_set_callback(ffmpegLogCallback);
        });
    }
}

bool FFmpegExtractor::supports(core::models::MediaType type, const std::string& extension) const {
    return type == core::models::MediaType::Video || type == core::models::MediaType::Audio;
}

core::Result<core::indexer::Metadata> FFmpegExtractor::extract(const std::string& canonicalPath, core::models::MediaType type) {
    if (!supports(type, "")) {
        return core::Error{-1, "Unsupported media type"};
    }

    initFFmpeg();

    AVFormatContext* fmt_ctx = nullptr;
    
    // Open the file
    int ret = avformat_open_input(&fmt_ctx, canonicalPath.c_str(), nullptr, nullptr);
    if (ret < 0) {
        return core::Error{-1, "FFmpeg failed to open format context"};
    }

    // Wrap the context in a smart pointer for safe cleanup (RAII)
    auto formatContextReleaser = [](AVFormatContext* ctx) {
        avformat_close_input(&ctx);
    };
    std::unique_ptr<AVFormatContext, decltype(formatContextReleaser)> ctxGuard(fmt_ctx, formatContextReleaser);

    // Read streams
    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        return core::Error{-1, "FFmpeg failed to find stream information"};
    }

    core::indexer::Metadata meta;

    if (fmt_ctx->duration != AV_NOPTS_VALUE) {
        meta.duration = static_cast<double>(fmt_ctx->duration) / AV_TIME_BASE;
    }

    // Extract basic dictionary tags from format context
    const AVDictionaryEntry* tag = nullptr;
    while ((tag = av_dict_iterate(fmt_ctx->metadata, tag))) {
        std::string key = tag->key;
        std::string val = tag->value;
        // simplistic lowercasing
        for (auto& c : key) c = std::tolower(c);

        if (key == "title") meta.title = val;
        else if (key == "artist") meta.artist = val;
        else if (key == "album") meta.album = val;
        else if (key == "album_artist") meta.albumArtist = val;
        else if (key == "genre") meta.genre = val;
        else if (key == "date") {
            unsigned int year;
            if (auto [p, ec] = std::from_chars(val.data(), val.data() + val.size(), year); ec == std::errc()) {
                meta.year = year;
            }
        }
        else if (key == "track") {
            unsigned int track;
            if (auto [p, ec] = std::from_chars(val.data(), val.data() + val.size(), track); ec == std::errc()) {
                meta.trackNumber = track;
            }
        }
    }

    // Find first video and audio stream
    bool videoFound = false;
    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        AVStream* stream = fmt_ctx->streams[i];
        AVCodecParameters* codecpar = stream->codecpar;

        if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !videoFound) {
            meta.width = codecpar->width;
            meta.height = codecpar->height;
            if (stream->avg_frame_rate.den > 0) {
                meta.frameRate = av_q2d(stream->avg_frame_rate);
            }
            videoFound = true;
        }
        else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            if (!meta.audioChannels) {
                meta.audioChannels = codecpar->ch_layout.nb_channels;
                meta.audioSampleRate = codecpar->sample_rate;
            }
        }
    }

    return meta;
}

} // namespace mnemis::indexer
