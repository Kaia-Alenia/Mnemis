#include "StbImageExtractor.hpp"
#include <cstdio>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_GIF
#define STBI_ONLY_BMP
#define STBI_NO_LINEAR
#include "third_party/stb/stb_image.h"

namespace mnemis::indexer {

bool StbImageExtractor::supports(core::models::MediaType type, const std::string& extension) const {
    return type == core::models::MediaType::Image || type == core::models::MediaType::Gif;
}

core::Result<core::indexer::Metadata> StbImageExtractor::extract(const std::string& canonicalPath, core::models::MediaType type) {
    if (!supports(type, "")) {
        return core::Error{-1, "Unsupported media type"};
    }

    int width = 0, height = 0, comp = 0;
    
    // stbi_info parses the header without decoding the entire image
    int ok = stbi_info(canonicalPath.c_str(), &width, &height, &comp);
    
    if (!ok) {
        return core::Error{-1, std::string("stb_image failed to read metadata: ") + stbi_failure_reason()};
    }

    core::indexer::Metadata meta;
    meta.width = width;
    meta.height = height;
    
    return meta;
}

} // namespace mnemis::indexer
