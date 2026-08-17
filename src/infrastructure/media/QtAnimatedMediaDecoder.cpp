#include "QtAnimatedMediaDecoder.hpp"
#include <QImage>
#include <QFileInfo>
#include <QDebug>

namespace mnemis::infrastructure::media {

bool QtAnimatedMediaDecoder::open(const std::string& filePath) {
    m_filePath = filePath;
    m_reader = std::make_unique<QImageReader>(QString::fromStdString(filePath));

    if (!m_reader->canRead()) {
        m_reader.reset();
        return false;
    }

    if (!m_reader->supportsAnimation()) {
        m_reader.reset();
        return false;
    }

    m_info.frameCount = m_reader->imageCount();
    
    int lc = m_reader->loopCount();
    if (lc < 0) {
        m_info.loopCount = 0; // Default to infinite for animated formats
    } else {
        m_info.loopCount = lc;
    }
    
    // Read dimensions from the first frame metadata
    QSize size = m_reader->size();
    m_info.width = size.width();
    m_info.height = size.height();

    // Calculate total duration by summing individual frame delays
    // For the first frame, nextImageDelay() returns its delay
    int firstDelay = m_reader->nextImageDelay();
    m_info.defaultFrameDurationMs = firstDelay > 0 ? firstDelay : 100;
    
    // Estimate total duration (sum of all frame delays)
    // We can't iterate without consuming frames, so estimate from default
    m_info.totalDurationMs = m_info.frameCount * m_info.defaultFrameDurationMs;

    m_currentIndex = -1;
    return true;
}

core::media::AnimatedMediaInfo QtAnimatedMediaDecoder::getInfo() const {
    return m_info;
}

std::optional<core::media::AnimatedFrame> QtAnimatedMediaDecoder::getFrame(int index) {
    if (!m_reader) {
        return std::nullopt;
    }

    // Try to jump or read sequentially
    if (index != m_currentIndex + 1) {
        if (!m_reader->jumpToImage(index)) {
            // Fallback: reopen and read sequentially to reach the target frame
            m_reader = std::make_unique<QImageReader>(QString::fromStdString(m_filePath));
            for (int i = 0; i < index; ++i) {
                if (m_reader->read().isNull()) {
                    return std::nullopt;
                }
            }
        }
    }

    int duration = m_reader->nextImageDelay();
    QImage image = m_reader->read();
    if (image.isNull()) {
        return std::nullopt;
    }

    m_currentIndex = index;

    // Convert to agnostic buffer (consistent RGBA8888 format)
    image = image.convertToFormat(QImage::Format_RGBA8888);

    core::media::AnimatedFrame frame;
    frame.index = index;
    frame.durationMs = duration > 0 ? duration : m_info.defaultFrameDurationMs;
    frame.width = image.width();
    frame.height = image.height();
    frame.stride = image.bytesPerLine();
    frame.pixelFormat = core::media::PixelFormat::RGBA8888;

    // Deep copy data
    const uint8_t* bits = image.constBits();
    size_t sizeInBytes = image.sizeInBytes();
    frame.data.assign(bits, bits + sizeInBytes);

    return frame;
}

} // namespace mnemis::infrastructure::media

