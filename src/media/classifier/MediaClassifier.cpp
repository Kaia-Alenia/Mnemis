#include "MediaClassifier.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace mnemis::media {

namespace {

using ExtensionList =
    std::initializer_list<std::string_view>;

constexpr ExtensionList kImageExtensions = {
    ".jpg",
    ".jpeg",
    ".jpe",
    ".png",
    ".gif",
    ".webp",
    ".bmp",
    ".tif",
    ".tiff",
    ".ico",
    ".avif",
    ".heic",
    ".heif",
    ".svg"
};

constexpr ExtensionList kVideoExtensions = {
    ".mp4",
    ".m4v",
    ".mkv",
    ".webm",
    ".avi",
    ".mov",
    ".wmv",
    ".flv",
    ".f4v",
    ".mpeg",
    ".mpg",
    ".mpe",
    ".ts",
    ".m2ts",
    ".mts",
    ".3gp",
    ".3g2",
    ".ogv"
};

constexpr ExtensionList kAudioExtensions = {
    ".mp3",
    ".wav",
    ".flac",
    ".ogg",
    ".oga",
    ".opus",
    ".m4a",
    ".aac",
    ".wma",
    ".alac",
    ".aiff",
    ".aif",
    ".ape",
    ".ac3",
    ".dts"
};

std::string normalizeExtension(
    std::string_view extension
)
{
    std::string normalized;

    normalized.reserve(extension.size() + 1);

    for (const char character : extension) {
        normalized.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character)
                )
            )
        );
    }

    if (!normalized.empty() &&
        normalized.front() != '.') {
        normalized.insert(
            normalized.begin(),
            '.'
        );
    }

    return normalized;
}

bool contains(
    ExtensionList extensions,
    std::string_view extension
) noexcept
{
    for (const auto candidate : extensions) {
        if (candidate == extension) {
            return true;
        }
    }

    return false;
}

std::vector<std::string> toVector(
    ExtensionList extensions
)
{
    std::vector<std::string> result;

    result.reserve(extensions.size());

    for (const auto extension : extensions) {
        result.emplace_back(extension);
    }

    return result;
}

} // namespace

MediaType MediaClassifier::classifyExtension(
    std::string_view extension
) const noexcept
{
    const std::string normalized =
        normalizeExtension(extension);

    if (contains(
            kImageExtensions,
            normalized)) {
        return MediaType::Image;
    }

    if (contains(
            kVideoExtensions,
            normalized)) {
        return MediaType::Video;
    }

    if (contains(
            kAudioExtensions,
            normalized)) {
        return MediaType::Audio;
    }

    return MediaType::Unknown;
}

core::Result<MediaType>
MediaClassifier::classifyPath(
    std::string_view path
) const
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "MediaClassifier",
            "Path cannot be empty"
        };
    }

    const std::filesystem::path filesystemPath{
        std::string(path)
    };

    const std::string extension =
        filesystemPath.extension().string();

    if (extension.empty()) {
        return core::Error{
            core::ErrorCode::Unsupported,
            "MediaClassifier",
            "Path has no file extension: " +
            std::string(path)
        };
    }

    const MediaType type =
        classifyExtension(extension);

    if (type == MediaType::Unknown) {
        return core::Error{
            core::ErrorCode::Unsupported,
            "MediaClassifier",
            "Unsupported media extension: " +
            extension
        };
    }

    return type;
}

bool MediaClassifier::isSupportedExtension(
    std::string_view extension
) const noexcept
{
    return classifyExtension(extension) !=
           MediaType::Unknown;
}

std::vector<std::string>
MediaClassifier::imageExtensions()
{
    return toVector(kImageExtensions);
}

std::vector<std::string>
MediaClassifier::videoExtensions()
{
    return toVector(kVideoExtensions);
}

std::vector<std::string>
MediaClassifier::audioExtensions()
{
    return toVector(kAudioExtensions);
}

} // namespace mnemis::media
