#pragma once

#include "core/media/IAnimatedMediaDecoder.hpp"
#include <QImageReader>
#include <memory>
#include <string>

namespace mnemis::infrastructure::media {

class QtAnimatedMediaDecoder : public core::media::IAnimatedMediaDecoder {
public:
    QtAnimatedMediaDecoder() = default;
    ~QtAnimatedMediaDecoder() override = default;

    bool open(const std::string& filePath) override;
    core::media::AnimatedMediaInfo getInfo() const override;
    std::optional<core::media::AnimatedFrame> getFrame(int index) override;

private:
    std::unique_ptr<QImageReader> m_reader;
    std::string m_filePath;
    core::media::AnimatedMediaInfo m_info;
    int m_currentIndex = -1;
};

} // namespace mnemis::infrastructure::media
