#include "FFmpegThumbnailDecoder.hpp"
#include <algorithm>
#include <memory>
#include <mutex>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <libavutil/log.h>
}

namespace mnemis::infrastructure::thumbnails {

namespace {
    std::once_flag ffmpeg_init_flag;
    void initFFmpeg() {
        std::call_once(ffmpeg_init_flag, []() {
            av_log_set_level(AV_LOG_QUIET);
        });
    }
}

FFmpegThumbnailDecoder::FFmpegThumbnailDecoder() {
    // List of common video formats
    m_supportedExtensions = {
        ".mp4", ".mkv", ".avi", ".mov", ".webm", ".flv", ".wmv", ".m4v"
    };
}

bool FFmpegThumbnailDecoder::canHandle(const std::string& extension) const {
    std::string lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
    return m_supportedExtensions.find(lowerExt) != m_supportedExtensions.end();
}

std::optional<core::thumbnails::ImageBuffer> FFmpegThumbnailDecoder::decode(const std::string& path, const core::thumbnails::ThumbnailSpec& spec) {
    initFFmpeg();

    AVFormatContext* fmt_ctx = nullptr;
    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0) {
        return std::nullopt;
    }
    
    auto formatContextReleaser = [](AVFormatContext* ctx) { avformat_close_input(&ctx); };
    std::unique_ptr<AVFormatContext, decltype(formatContextReleaser)> ctxGuard(fmt_ctx, formatContextReleaser);

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        return std::nullopt;
    }

    int video_stream_idx = -1;
    AVCodecParameters* codecpar = nullptr;
    const AVCodec* codec = nullptr;

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; i++) {
        if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = i;
            codecpar = fmt_ctx->streams[i]->codecpar;
            codec = avcodec_find_decoder(codecpar->codec_id);
            break;
        }
    }

    if (video_stream_idx == -1 || !codec) {
        return std::nullopt;
    }

    AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        return std::nullopt;
    }
    auto codecCtxReleaser = [](AVCodecContext* ctx) { avcodec_free_context(&ctx); };
    std::unique_ptr<AVCodecContext, decltype(codecCtxReleaser)> codecCtxGuard(codec_ctx, codecCtxReleaser);

    if (avcodec_parameters_to_context(codec_ctx, codecpar) < 0) {
        return std::nullopt;
    }

    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        return std::nullopt;
    }

    // Seek to 1 second in or half the video, whatever is smaller, to avoid black frames.
    int64_t seek_target = AV_TIME_BASE; // 1 second
    if (fmt_ctx->duration > 0 && fmt_ctx->duration < AV_TIME_BASE) {
        seek_target = fmt_ctx->duration / 2;
    }
    avformat_seek_file(fmt_ctx, -1, INT64_MIN, seek_target, INT64_MAX, AVSEEK_FLAG_BACKWARD);

    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    auto framePacketReleaser = [&](void*) { av_frame_free(&frame); av_packet_free(&packet); };
    std::unique_ptr<void, decltype(framePacketReleaser)> fpGuard(reinterpret_cast<void*>(1), framePacketReleaser);

    bool frame_decoded = false;
    int max_packets_to_read = 100; // safety limit
    int packets_read = 0;

    while (packets_read < max_packets_to_read && av_read_frame(fmt_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            if (avcodec_send_packet(codec_ctx, packet) == 0) {
                if (avcodec_receive_frame(codec_ctx, frame) == 0) {
                    frame_decoded = true;
                    av_packet_unref(packet);
                    break;
                }
            }
        }
        av_packet_unref(packet);
        packets_read++;
    }

    if (!frame_decoded) {
        return std::nullopt;
    }

    // Determine target size while maintaining aspect ratio
    int orig_width = frame->width;
    int orig_height = frame->height;
    if (orig_width <= 0 || orig_height <= 0) return std::nullopt;
    
    double aspect_ratio = static_cast<double>(orig_width) / orig_height;
    int target_width = spec.targetWidth;
    int target_height = static_cast<int>(target_width / aspect_ratio);
    
    if (target_height > static_cast<int>(spec.targetHeight)) {
        target_height = spec.targetHeight;
        target_width = static_cast<int>(target_height * aspect_ratio);
    }
    
    // Scale and convert to RGB24 using swscale
    SwsContext* sws_ctx = sws_getContext(
        orig_width, orig_height, codec_ctx->pix_fmt,
        target_width, target_height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr
    );

    if (!sws_ctx) {
        return std::nullopt;
    }
    
    auto swsCtxReleaser = [](SwsContext* ctx) { sws_freeContext(ctx); };
    std::unique_ptr<SwsContext, decltype(swsCtxReleaser)> swsGuard(sws_ctx, swsCtxReleaser);

    AVFrame* rgb_frame = av_frame_alloc();
    rgb_frame->format = AV_PIX_FMT_RGB24;
    rgb_frame->width = target_width;
    rgb_frame->height = target_height;
    av_frame_get_buffer(rgb_frame, 0);

    sws_scale(sws_ctx, frame->data, frame->linesize, 0, orig_height, rgb_frame->data, rgb_frame->linesize);

    core::thumbnails::ImageBuffer buffer;
    buffer.width = target_width;
    buffer.height = target_height;
    buffer.channels = 3;
    buffer.isEncoded = false;
    
    // Copy rgb data
    int size = target_width * target_height * 3;
    // linesize could be larger than width*3 due to alignment
    buffer.data.reserve(size);
    for (int y = 0; y < target_height; ++y) {
        const uint8_t* row = rgb_frame->data[0] + y * rgb_frame->linesize[0];
        buffer.data.insert(buffer.data.end(), row, row + target_width * 3);
    }

    av_frame_free(&rgb_frame);

    return buffer;
}

} // namespace mnemis::infrastructure::thumbnails
