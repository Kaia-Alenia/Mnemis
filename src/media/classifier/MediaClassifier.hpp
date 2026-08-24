#pragma once

#include "media/models/MediaType.hpp"
#include "core/errors/Result.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mnemis::media {

class MediaClassifier final {
public:
    MediaClassifier() = default;

    MediaType classifyExtension(
        std::string_view extension
    ) const noexcept;

    core::Result<MediaType> classifyPath(
        std::string_view path
    ) const;

    bool isSupportedExtension(
        std::string_view extension
    ) const noexcept;

    static std::vector<std::string> imageExtensions();
    static std::vector<std::string> videoExtensions();
    static std::vector<std::string> audioExtensions();
};

} // namespace mnemis::media
