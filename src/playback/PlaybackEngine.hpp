#pragma once

#include "playback/IPlaybackBackend.hpp"
#include "playback/PlaybackTypes.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>

namespace mnemis::playback {

class PlaybackEngine : public IPlaybackBackendDelegate {
public:
    explicit PlaybackEngine(std::unique_ptr<IPlaybackBackend> backend);
    ~PlaybackEngine() override;

    // Commands
    void load(const std::string& uri, PlaybackOrigin::Value origin);
    void play(PlaybackOrigin::Value origin);
    void pause(PlaybackOrigin::Value origin);
    void stop(PlaybackOrigin::Value origin);
    void seek(double positionSecs, PlaybackOrigin::Value origin);
    void setVolume(double value);
    void setPlaybackRate(double rate);

    // Queries
    PlaybackState::Value state() const;
    PlaybackOrigin::Value currentOrigin() const;
    double position() const;
    double duration() const;
    bool isBuffering() const;
    const std::vector<TrackInfo>& audioTracks() const;
    const std::vector<TrackInfo>& subtitleTracks() const;
    std::string currentMedia() const;

    // Callbacks to controller
    using StateChangedCb = std::function<void(PlaybackState::Value)>;
    using PositionChangedCb = std::function<void(double)>;
    using DurationChangedCb = std::function<void(double)>;
    using TracksChangedCb = std::function<void()>;
    using ErrorCb = std::function<void(const std::string&)>;
    using BufferingChangedCb = std::function<void(bool)>;

    void setStateChangedCallback(StateChangedCb cb);
    void setPositionChangedCallback(PositionChangedCb cb);
    void setDurationChangedCallback(DurationChangedCb cb);
    void setTracksChangedCallback(TracksChangedCb cb);
    void setErrorCallback(ErrorCb cb);
    void setBufferingChangedCallback(BufferingChangedCb cb);

    // IPlaybackBackendDelegate
    void onStateChanged(PlaybackState::Value state) override;
    void onPositionChanged(double positionSecs) override;
    void onDurationChanged(double durationSecs) override;
    void onTracksChanged(const std::vector<TrackInfo>& audioTracks, const std::vector<TrackInfo>& subtitleTracks) override;
    void onError(const std::string& message) override;
    void onBufferingChanged(bool isBuffering) override;
    void onEnded() override;

private:
    std::unique_ptr<IPlaybackBackend> m_backend;
    
    mutable std::recursive_mutex m_mutex;
    PlaybackState::Value m_state = PlaybackState::Idle;
    PlaybackOrigin::Value m_currentOrigin = PlaybackOrigin::Internal;
    std::string m_currentMedia;
    double m_position = 0.0;
    double m_duration = 0.0;
    bool m_isBuffering = false;
    std::vector<TrackInfo> m_audioTracks;
    std::vector<TrackInfo> m_subtitleTracks;
    
    std::atomic<int> m_loadGeneration{0};
    int m_activeGeneration = 0;

    StateChangedCb m_stateCb;
    PositionChangedCb m_posCb;
    DurationChangedCb m_durCb;
    TracksChangedCb m_tracksCb;
    ErrorCb m_errorCb;
    BufferingChangedCb m_buffCb;

    bool isValidTransition(PlaybackState::Value from, PlaybackState::Value to) const;
};

} // namespace mnemis::playback
