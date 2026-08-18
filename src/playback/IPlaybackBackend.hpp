#pragma once

#include <string>
#include <vector>
#include <variant>
#include <map>
#include "playback/PlaybackTypes.hpp"

namespace mnemis::playback {

using TrackInfo = std::map<std::string, std::variant<int, std::string, bool>>;

class IPlaybackBackendDelegate {
public:
    virtual ~IPlaybackBackendDelegate() = default;

    virtual void onStateChanged(PlaybackState::Value state) = 0;
    virtual void onPositionChanged(double positionSecs) = 0;
    virtual void onDurationChanged(double durationSecs) = 0;
    virtual void onTracksChanged(const std::vector<TrackInfo>& audioTracks, const std::vector<TrackInfo>& subtitleTracks) = 0;
    virtual void onError(const std::string& message) = 0;
    virtual void onBufferingChanged(bool isBuffering) = 0;
    virtual void onEnded() = 0;
};

class IPlaybackBackend {
public:
    virtual ~IPlaybackBackend() = default;

    virtual void setDelegate(IPlaybackBackendDelegate* delegate) = 0;
    
    virtual void load(const std::string& uri) = 0;
    virtual void play() = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual void seek(double positionSecs) = 0;
    virtual void setVolume(double value) = 0;
    virtual void setPlaybackRate(double rate) = 0;
    virtual void* getNativePlayer() const = 0;
};

} // namespace mnemis::playback
