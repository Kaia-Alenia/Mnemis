#include "AnimatedMediaController.hpp"
#include "infrastructure/media/QtAnimatedMediaDecoder.hpp"
#include <QTimerEvent>
#include <algorithm>

namespace mnemis::ui::controllers {

AnimatedMediaController::AnimatedMediaController(QObject* parent)
    : QObject(parent)
{
}

AnimatedMediaController::~AnimatedMediaController() {
    clear();
}

QString AnimatedMediaController::animationState() const {
    switch (m_state) {
        case AnimationState::Unavailable: return "Unavailable";
        case AnimationState::Loading:     return "Loading";
        case AnimationState::Ready:       return "Ready";
        case AnimationState::Playing:     return "Playing";
        case AnimationState::Paused:      return "Paused";
        case AnimationState::Error:       return "Error";
    }
    return "Unavailable";
}

bool AnimatedMediaController::isAnimated() const {
    return m_decoder != nullptr;
}

bool AnimatedMediaController::isPlaying() const {
    return m_state == AnimationState::Playing;
}

int AnimatedMediaController::currentFrameIndex() const {
    return m_currentFrameIndex;
}

int AnimatedMediaController::frameCount() const {
    return m_info.frameCount;
}

qreal AnimatedMediaController::speed() const {
    return m_speed;
}

void AnimatedMediaController::setSpeed(qreal speed) {
    qreal clamped = std::clamp(speed, kMinSpeed, kMaxSpeed);
    if (qFuzzyCompare(m_speed, clamped)) return;
    m_speed = clamped;
    emit speedChanged();
}

void AnimatedMediaController::loadMedia(const QString& filePath) {
    clear();

    setState(AnimationState::Loading);

    auto decoder = std::make_unique<infrastructure::media::QtAnimatedMediaDecoder>();
    if (!decoder->open(filePath.toStdString())) {
        setState(AnimationState::Error);
        return;
    }

    m_info = decoder->getInfo();
    if (m_info.frameCount <= 0) {
        setState(AnimationState::Error);
        return;
    }

    m_decoder = std::move(decoder);
    m_currentFrameIndex = -1;
    m_currentLoop = 0;

    emit isAnimatedChanged();
    emit frameCountChanged();
    
    setState(AnimationState::Ready);

    // Auto-play when loaded
    play();
}

void AnimatedMediaController::play() {
    if (!m_decoder) return;
    if (m_state == AnimationState::Playing) return;

    setState(AnimationState::Playing);
    
    // Decode the next frame (or first frame if starting fresh)
    int nextIndex = (m_currentFrameIndex < 0) ? 0 : m_currentFrameIndex + 1;
    if (m_info.frameCount > 0 && nextIndex >= m_info.frameCount) {
        nextIndex = 0;
    }
    decodeAndEmitFrame(nextIndex);
}

void AnimatedMediaController::pause() {
    if (m_state != AnimationState::Playing) return;
    stopTimer();
    setState(AnimationState::Paused);
}

void AnimatedMediaController::stop() {
    qInfo() << "[ANIMATED] stop() called \u2014 clearing animation state";
    clear();
}

void AnimatedMediaController::restart() {
    if (!m_decoder) return;
    stopTimer();
    m_currentFrameIndex = -1;
    m_currentLoop = 0;
    emit currentFrameIndexChanged();
    setState(AnimationState::Ready);
    play();
}

void AnimatedMediaController::goToFrame(int index) {
    if (!m_decoder) return;
    if (index < 0 || index >= m_info.frameCount) return;
    
    // Pause if playing
    if (m_state == AnimationState::Playing) {
        stopTimer();
    }

    decodeAndEmitFrame(index);
    setState(AnimationState::Paused);
}

void AnimatedMediaController::clear() {
    stopTimer();
    
    bool wasAnimated = isAnimated();
    AnimationState oldState = m_state;
    int oldFrameCount = frameCount();
    int oldFrameIndex = m_currentFrameIndex;
    
    m_decoder.reset();
    m_info = core::media::AnimatedMediaInfo{};
    m_state = AnimationState::Unavailable;
    m_currentFrameIndex = -1;
    m_currentLoop = 0;
    
    if (wasAnimated) emit isAnimatedChanged();
    if (oldState != m_state) {
        emit animationStateChanged();
        emit isPlayingChanged();
    }
    if (oldFrameCount != 0) emit frameCountChanged();
    if (oldFrameIndex != -1) emit currentFrameIndexChanged();
}

void AnimatedMediaController::decodeAndEmitFrame(int index) {
    if (!m_decoder) return;

    auto optFrame = m_decoder->getFrame(index);
    if (!optFrame) {
        // Try frame 0 as fallback
        if (index != 0) {
            optFrame = m_decoder->getFrame(0);
        }
        if (!optFrame) {
            stopTimer();
            setState(AnimationState::Error);
            return;
        }
    }

    const auto& frame = *optFrame;
    m_currentFrameIndex = frame.index;
    emit currentFrameIndexChanged();

    // Convert core frame to QImage
    QImage::Format qFormat = QImage::Format_Invalid;
    switch (frame.pixelFormat) {
        case core::media::PixelFormat::RGBA8888:
            qFormat = QImage::Format_RGBA8888;
            break;
        case core::media::PixelFormat::RGB888:
            qFormat = QImage::Format_RGB888;
            break;
        case core::media::PixelFormat::Grayscale8:
            qFormat = QImage::Format_Grayscale8;
            break;
        default:
            break;
    }

    if (qFormat != QImage::Format_Invalid && !frame.data.empty()) {
        QImage temp(frame.data.data(), frame.width, frame.height, frame.stride, qFormat);
        emit frameReady(temp.copy()); // Deep copy to detach from core buffer
    }

    // Schedule next frame only if playing
    if (m_state == AnimationState::Playing) {
        // Check if we need to handle end-of-animation / loops
        int nextIndex = m_currentFrameIndex + 1;
        if (m_info.frameCount > 0 && nextIndex >= m_info.frameCount) {
            // End of a loop
            m_currentLoop++;
            if (m_info.loopCount > 0 && m_currentLoop >= m_info.loopCount) {
                // Finite loop exhausted
                setState(AnimationState::Paused);
                return;
            }
            // Infinite loop or more loops remaining — will wrap in next call
        }
        scheduleNextFrame(frame.durationMs);
    }
}

void AnimatedMediaController::scheduleNextFrame(int delayMs) {
    stopTimer();
    int adjusted = clampDelay(static_cast<int>(delayMs / m_speed));
    m_timerId = startTimer(adjusted);
}

void AnimatedMediaController::stopTimer() {
    if (m_timerId != 0) {
        killTimer(m_timerId);
        m_timerId = 0;
    }
}

void AnimatedMediaController::setState(AnimationState newState) {
    if (m_state == newState) return;
    bool wasPlaying = isPlaying();
    m_state = newState;
    emit animationStateChanged();
    if (wasPlaying != isPlaying()) {
        emit isPlayingChanged();
    }
}

int AnimatedMediaController::clampDelay(int delayMs) const {
    if (delayMs < kMinFrameDelayMs) return kMinFrameDelayMs;
    if (delayMs > kMaxFrameDelayMs) return kMaxFrameDelayMs;
    return delayMs;
}

void AnimatedMediaController::timerEvent(QTimerEvent* event) {
    if (event->timerId() == m_timerId) {
        stopTimer(); // Kill before decoding to avoid re-entrance
        
        int nextIndex = m_currentFrameIndex + 1;
        if (m_info.frameCount > 0 && nextIndex >= m_info.frameCount) {
            nextIndex = 0; // Wrap for loop
        }
        decodeAndEmitFrame(nextIndex);
    } else {
        QObject::timerEvent(event);
    }
}

} // namespace mnemis::ui::controllers
