#pragma once

#include "MediaId.hpp"
#include "MediaType.hpp"

#include <cstdint>
#include <string>

namespace mnemis::media {

struct Media final {
    MediaId id = InvalidMediaId;

    MediaType type = MediaType::Unknown;

    std::string canonicalPath;
    std::string fileName;
    std::string extension;

    std::int64_t fileSize = 0;
    std::int64_t modifiedTime = 0;

    bool favorite = false;

    bool isValid() const noexcept
    {
        return isValidMediaId(id) &&
               type != MediaType::Unknown &&
               !canonicalPath.empty();
    }

    bool isPlayable() const noexcept
    {
        return media::isPlayable(type);
    }

    bool isVisual() const noexcept
    {
        return media::isVisual(type);
    }
};

} // namespace mnemis::media
