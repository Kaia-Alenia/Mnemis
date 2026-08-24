#pragma once

#include <cstdint>

namespace mnemis::media {

enum class MediaType : std::uint8_t {
    Unknown = 0,
    Image,
    Video,
    Audio
};

constexpr const char* mediaTypeName(MediaType type) noexcept
{
    switch (type) {
        case MediaType::Unknown:
            return "Unknown";

        case MediaType::Image:
            return "Image";

        case MediaType::Video:
            return "Video";

        case MediaType::Audio:
            return "Audio";
    }

    return "Unknown";
}

constexpr bool isVisual(MediaType type) noexcept
{
    return type == MediaType::Image ||
           type == MediaType::Video;
}

constexpr bool isPlayable(MediaType type) noexcept
{
    return type == MediaType::Video ||
           type == MediaType::Audio;
}

} // namespace mnemis::media
