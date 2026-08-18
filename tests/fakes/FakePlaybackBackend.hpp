#pragma once

#include "playback/IPlaybackBackend.hpp"
#include <string>

namespace mnemis::tests::fakes {

class FakePlaybackBackend : public playback::IPlaybackBackend {
public:
    FakePlaybackBackend() = default;
    ~FakePlaybackBackend() override = default;

    void setDelegate(playback::IPlaybackBackendDelegate* delegate) override {
        m_delegate = delegate;
    }

    void load(const std::string& uri) override {
        m_lastUri = uri;
        if (m_delegate) {
            m_delegate->onStateChanged(playback::PlaybackState::Loading);
            if (m_shouldFailLoad) {
                m_delegate->onError("Failed to load");
                m_delegate->onStateChanged(playback::PlaybackState::Error);
            } else {
                m_delegate->onStateChanged(playback::PlaybackState::Ready);
                m_delegate->onDurationChanged(m_simulatedDuration);
            }
        }
    }

    void play() override {
        if (m_delegate) {
            m_delegate->onStateChanged(playback::PlaybackState::Playing);
        }
    }

    void pause() override {
        if (m_delegate) {
            m_delegate->onStateChanged(playback::PlaybackState::Paused);
        }
    }

    void stop() override {
        if (m_delegate) {
            m_delegate->onStateChanged(playback::PlaybackState::Stopping);
            m_delegate->onStateChanged(playback::PlaybackState::Idle);
        }
    }

    void seek(double positionSecs) override {
        if (m_delegate) {
            m_delegate->onStateChanged(playback::PlaybackState::Seeking);
            m_delegate->onPositionChanged(positionSecs);
            // In a real scenario, it returns to the previous state. For simplicity in the fake, we can just jump back to paused if not handled explicitly.
            m_delegate->onStateChanged(playback::PlaybackState::Paused);
        }
    }

    void setVolume(double value) override {
        m_volume = value;
    }

    void setPlaybackRate(double rate) override {
        m_playbackRate = rate;
    }

    void* getNativePlayer() const override {
        return nullptr;
    }

    // Simulation controls
    void setShouldFailLoad(bool fail) { m_shouldFailLoad = fail; }
    void setSimulatedDuration(double duration) { m_simulatedDuration = duration; }
    void simulateStateChange(playback::PlaybackState::Value state) { if (m_delegate) m_delegate->onStateChanged(state); }
    void simulatePositionChanged(double pos) { if (m_delegate) m_delegate->onPositionChanged(pos); }
    void simulateEnded() { if (m_delegate) { m_delegate->onEnded(); m_delegate->onStateChanged(playback::PlaybackState::Ended); } }
    void simulateError(const std::string& err) { if (m_delegate) { m_delegate->onError(err); m_delegate->onStateChanged(playback::PlaybackState::Error); } }
    void simulateBuffering(bool buff) { if (m_delegate) m_delegate->onBufferingChanged(buff); }
    void simulateTracksChanged(const std::vector<playback::TrackInfo>& audio, const std::vector<playback::TrackInfo>& subs) {
        if (m_delegate) m_delegate->onTracksChanged(audio, subs);
    }

    std::string getLastUri() const { return m_lastUri; }
    double getVolume() const { return m_volume; }
    double getPlaybackRate() const { return m_playbackRate; }

private:
    playback::IPlaybackBackendDelegate* m_delegate = nullptr;
    std::string m_lastUri;
    bool m_shouldFailLoad = false;
    double m_simulatedDuration = 0.0;
    double m_volume = 1.0;
    double m_playbackRate = 1.0;
};

} // namespace mnemis::tests::fakes
