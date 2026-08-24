#include "StbThumbnailDecoder.hpp"
#include <QImage>
#include <algorithm>

// Define stb_image implementation locally
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
// Ensure stb_image doesn't use static variables that might conflict if defined elsewhere
#define STBI_NO_FAILURE_STRINGS
#include "../../third_party/stb/stb_image.h"

namespace mnemis::infrastructure::thumbnails {

StbThumbnailDecoder::StbThumbnailDecoder() {
    m_supportedExtensions = {
        ".jpg", ".jpeg", ".png", ".bmp", ".tga", ".gif", ".hdr", ".pic", ".pnm"
    };
}

bool StbThumbnailDecoder::canHandle(const std::string& extension) const {
    std::string lowerExt = extension;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);
    return m_supportedExtensions.find(lowerExt) != m_supportedExtensions.end();
}

std::optional<core::thumbnails::ImageBuffer> StbThumbnailDecoder::decode(const std::string& path, const core::thumbnails::ThumbnailSpec& spec) {
    int width = 0;
    int height = 0;
    int channels = 0;
    
    // We request 4 channels (RGBA) for simplicity, or we can leave it to original
    // but 0 means use original channels.
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        return std::nullopt;
    }
    
    // Scale the image using Qt to avoid adding stb_image_resize dependency.
    QImage::Format fmt;
    if (channels == 1) {
        fmt = QImage::Format_Grayscale8;
    } else if (channels == 3) {
        fmt = QImage::Format_RGB888;
    } else if (channels == 4) {
        fmt = QImage::Format_RGBA8888;
    } else {
        stbi_image_free(data);
        return std::nullopt; // Unsupported channels
    }
    
    // QImage doesn't copy data here, it wraps the pointer
    QImage origImg(data, width, height, width * channels, fmt);
    
    // Scale image
    QImage scaledImg = origImg.scaled(spec.targetWidth, spec.targetHeight, 
                                      Qt::KeepAspectRatio, Qt::FastTransformation);
    
    // Prepare the final ImageBuffer (raw pixels)
    core::thumbnails::ImageBuffer buffer;
    buffer.width = scaledImg.width();
    buffer.height = scaledImg.height();
    buffer.channels = scaledImg.hasAlphaChannel() ? 4 : 3;
    buffer.isEncoded = false; // it's raw pixels
    
    // Convert scaled image to final format if needed
    QImage finalImg = scaledImg;
    if (buffer.channels == 4 && finalImg.format() != QImage::Format_RGBA8888) {
        finalImg = finalImg.convertToFormat(QImage::Format_RGBA8888);
    } else if (buffer.channels == 3 && finalImg.format() != QImage::Format_RGB888) {
        finalImg = finalImg.convertToFormat(QImage::Format_RGB888);
    }
    
    // Copy the pixel data out of QImage into our buffer
    const uint8_t* bits = finalImg.constBits();
    int byteCount = finalImg.sizeInBytes();
    buffer.data.assign(bits, bits + byteCount);
    
    // Free the original stbi data
    stbi_image_free(data);
    
    return buffer;
}

} // namespace mnemis::infrastructure::thumbnails
