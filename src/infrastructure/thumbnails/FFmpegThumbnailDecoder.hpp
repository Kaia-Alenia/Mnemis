#pragma once

#include "core/thumbnails/IThumbnailDecoder.hpp"
#include <string>
#include <unordered_set>

namespace mnemis::infrastructure::thumbnails {

class FFmpegThumbnailDecoder : public core::thumbnails::IThumbnailDecoder {
public:
    FFmpegThumbnailDecoder();
    ~FFmpegThumbnailDecoder() override = default;

    std::optional<core::thumbnails::ImageBuffer> decode(const std::string& path, const core::thumbnails::ThumbnailSpec& spec) override;
    
    bool canHandle(const std::string& extension) const override;

private:
    std::unordered_set<std::string> m_supportedExtensions;
};

} // namespace mnemis::infrastructure::thumbnails
