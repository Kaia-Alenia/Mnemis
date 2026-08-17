#pragma once

#include <QObject>
#include <QString>
#include <QImage>
#include <memory>

#include "core/media/IAnimatedMediaDecoder.hpp"
#include "core/media/AnimatedMediaInfo.hpp"

namespace mnemis::ui::controllers {

class AnimatedMediaController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString animationState READ animationState NOTIFY animationStateChanged)
    Q_PROPERTY(bool isAnimated READ isAnimated NOTIFY isAnimatedChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY isPlayingChanged)
    Q_PROPERTY(int currentFrameIndex READ currentFrameIndex NOTIFY currentFrameIndexChanged)
    Q_PROPERTY(int frameCount READ frameCount NOTIFY frameCountChanged)
    Q_PROPERTY(qreal speed READ speed WRITE setSpeed NOTIFY speedChanged)

public:
    enum class AnimationState {
        Unavailable,
        Loading,
        Ready,
        Playing,
        Paused,
        Error
    };
    Q_ENUM(AnimationState)

    explicit AnimatedMediaController(QObject* parent = nullptr);
    ~AnimatedMediaController() override;

    QString animationState() const;
    bool isAnimated() const;
    bool isPlaying() const;
    int currentFrameIndex() const;
    int frameCount() const;
    qreal speed() const;
    void setSpeed(qreal speed);

    Q_INVOKABLE void loadMedia(const QString& filePath);
    Q_INVOKABLE void play();
    Q_INVOKABLE void pause();
    Q_INVOKABLE void restart();
    Q_INVOKABLE void goToFrame(int index);

signals:
    void animationStateChanged();
    void isPlayingChanged();
    void isAnimatedChanged();
    void currentFrameIndexChanged();
    void frameCountChanged();
    void speedChanged();
    void frameReady(const QImage& frameImage);

private:
    static constexpr int kMinFrameDelayMs = 10;
    static constexpr int kMaxFrameDelayMs = 10000;
    static constexpr qreal kMinSpeed = 0.1;
    static constexpr qreal kMaxSpeed = 4.0;

    std::unique_ptr<core::media::IAnimatedMediaDecoder> m_decoder;
    core::media::AnimatedMediaInfo m_info;
    AnimationState m_state = AnimationState::Unavailable;
    int m_currentFrameIndex = -1;
    int m_currentLoop = 0;
    qreal m_speed = 1.0;
    int m_timerId = 0;

    void timerEvent(QTimerEvent* event) override;
    void decodeAndEmitFrame(int index);
    void scheduleNextFrame(int delayMs);
    void setState(AnimationState newState);
    void stopTimer();
    void clear();

    int clampDelay(int delayMs) const;
};

} // namespace mnemis::ui::controllers
