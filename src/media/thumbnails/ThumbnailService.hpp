#pragma once

#include "core/errors/Result.hpp"
#include "media/models/Media.hpp"

#include <filesystem>

namespace mnemis::media {

class ThumbnailService final {
public:
    explicit ThumbnailService(
        std::filesystem::path cacheDirectory
    );

    core::Result<std::filesystem::path> thumbnailFor(
        const Media& media,
        int maxWidth,
        int maxHeight
    );

    core::Result<void> clearCache();

    const std::filesystem::path& cacheDirectory() const noexcept;

private:
    std::filesystem::path m_cacheDirectory;

    core::Result<void> ensureCacheDirectory() const;

    std::filesystem::path cachePathFor(
        const Media& media,
        int maxWidth,
        int maxHeight
    ) const;

    core::Result<std::filesystem::path> generateImageThumbnail(
        const Media& media,
        int maxWidth,
        int maxHeight
    );
};

} // namespace mnemis::media
