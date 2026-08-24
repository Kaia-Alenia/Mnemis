#include "playback/PlaybackEngine.hpp"

namespace mnemis::playback {

PlaybackEngine::PlaybackEngine(std::unique_ptr<IPlaybackBackend> backend)
    : m_backend(std::move(backend)) {
    if (m_backend) {
        m_backend->setDelegate(this);
    }
}

PlaybackEngine::~PlaybackEngine() {
    if (m_backend) {
        m_backend->setDelegate(nullptr);
    }
}

void PlaybackEngine::load(const std::string& uri, PlaybackOrigin::Value origin) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentOrigin = origin;
    m_currentMedia = uri;
    m_activeGeneration = ++m_loadGeneration;
    
    m_position = 0.0;
    m_duration = 0.0;
    m_isBuffering = false;
    m_audioTracks.clear();
    m_subtitleTracks.clear();
    m_state = PlaybackState::Loading;
    
    if (m_stateCb) {
        m_stateCb(m_state);
    }

    if (m_backend) {
        m_backend->load(uri);
    }
}

void PlaybackEngine::play(PlaybackOrigin::Value origin) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentOrigin = origin;
    if (m_backend) {
        m_backend->play();
    }
}

void PlaybackEngine::pause(PlaybackOrigin::Value origin) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentOrigin = origin;
    if (m_backend) {
        m_backend->pause();
    }
}

void PlaybackEngine::stop(PlaybackOrigin::Value origin) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentOrigin = origin;
    m_activeGeneration = ++m_loadGeneration; // Invalidate any loading in progress
    if (m_backend) {
        m_backend->stop();
    }
}

void PlaybackEngine::seek(double positionSecs, PlaybackOrigin::Value origin) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_currentOrigin = origin;
    if (m_backend) {
        m_backend->seek(positionSecs);
    }
}

void PlaybackEngine::setVolume(double value) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_backend) {
        m_backend->setVolume(value);
    }
}

void PlaybackEngine::setPlaybackRate(double rate) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_backend) {
        m_backend->setPlaybackRate(rate);
    }
}

void PlaybackEngine::setAudioTrack(int id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_backend) {
        m_backend->setAudioTrack(id);
    }
}

void PlaybackEngine::setSubtitleTrack(int id) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_backend) {
        m_backend->setSubtitleTrack(id);
    }
}

PlaybackState::Value PlaybackEngine::state() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_state;
}

PlaybackOrigin::Value PlaybackEngine::currentOrigin() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentOrigin;
}

double PlaybackEngine::position() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_position;
}

double PlaybackEngine::duration() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_duration;
}

bool PlaybackEngine::isBuffering() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_isBuffering;
}

const std::vector<TrackInfo>& PlaybackEngine::audioTracks() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_audioTracks;
}

const std::vector<TrackInfo>& PlaybackEngine::subtitleTracks() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_subtitleTracks;
}

std::string PlaybackEngine::currentMedia() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_currentMedia;
}

void* PlaybackEngine::getNativePlayer() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_backend) {
        return m_backend->getNativePlayer();
    }
    return nullptr;
}

void PlaybackEngine::setStateChangedCallback(StateChangedCb cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_stateCb = std::move(cb);
}

void PlaybackEngine::setPositionChangedCallback(PositionChangedCb cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_posCb = std::move(cb);
}

void PlaybackEngine::setDurationChangedCallback(DurationChangedCb cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_durCb = std::move(cb);
}

void PlaybackEngine::setTracksChangedCallback(TracksChangedCb cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_tracksCb = std::move(cb);
}

void PlaybackEngine::setErrorCallback(ErrorCb cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_errorCb = std::move(cb);
}

void PlaybackEngine::setBufferingChangedCallback(BufferingChangedCb cb) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_buffCb = std::move(cb);
}

void PlaybackEngine::onStateChanged(PlaybackState::Value state) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!isValidTransition(m_state, state)) {
        return;
    }
    m_state = state;
    if (m_stateCb) {
        m_stateCb(m_state);
    }
}

void PlaybackEngine::onPositionChanged(double positionSecs) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_position = positionSecs;
    if (m_posCb) {
        m_posCb(m_position);
    }
}

void PlaybackEngine::onDurationChanged(double durationSecs) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_duration = durationSecs;
    if (m_durCb) {
        m_durCb(m_duration);
    }
}

void PlaybackEngine::onTracksChanged(const std::vector<TrackInfo>& audioTracks, const std::vector<TrackInfo>& subtitleTracks) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_audioTracks = audioTracks;
    m_subtitleTracks = subtitleTracks;
    if (m_tracksCb) {
        m_tracksCb();
    }
}

void PlaybackEngine::onError(const std::string& message) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_state = PlaybackState::Error;
    if (m_errorCb) {
        m_errorCb(message);
    }
    if (m_stateCb) {
        m_stateCb(m_state);
    }
}

void PlaybackEngine::onBufferingChanged(bool isBuffering) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    m_isBuffering = isBuffering;
    if (m_buffCb) {
        m_buffCb(m_isBuffering);
    }
}

void PlaybackEngine::onEnded() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (isValidTransition(m_state, PlaybackState::Ended)) {
        m_state = PlaybackState::Ended;
        if (m_stateCb) {
            m_stateCb(m_state);
        }
    }
}

bool PlaybackEngine::isValidTransition(PlaybackState::Value from, PlaybackState::Value to) const {
    // Implement state machine validation logic
    if (from == to) return true; // Allowed
    
    switch (from) {
        case PlaybackState::Idle:
            return to == PlaybackState::Loading;
        case PlaybackState::Loading:
            return to == PlaybackState::Ready || to == PlaybackState::Playing || to == PlaybackState::Error || to == PlaybackState::Idle || to == PlaybackState::Stopping;
        case PlaybackState::Ready:
            return to == PlaybackState::Playing || to == PlaybackState::Paused || to == PlaybackState::Stopping || to == PlaybackState::Loading;
        case PlaybackState::Playing:
            return to == PlaybackState::Paused || to == PlaybackState::Seeking || to == PlaybackState::Buffering || to == PlaybackState::Stopping || to == PlaybackState::Ended || to == PlaybackState::Error || to == PlaybackState::Loading;
        case PlaybackState::Paused:
            return to == PlaybackState::Playing || to == PlaybackState::Seeking || to == PlaybackState::Stopping || to == PlaybackState::Loading;
        case PlaybackState::Seeking:
            return to == PlaybackState::Playing || to == PlaybackState::Paused || to == PlaybackState::Error || to == PlaybackState::Stopping || to == PlaybackState::Loading;
        case PlaybackState::Buffering:
            return to == PlaybackState::Playing || to == PlaybackState::Paused || to == PlaybackState::Error || to == PlaybackState::Stopping || to == PlaybackState::Loading;
        case PlaybackState::Stopping:
            return to == PlaybackState::Idle || to == PlaybackState::Ended || to == PlaybackState::Error || to == PlaybackState::Loading;
        case PlaybackState::Ended:
            return to == PlaybackState::Loading || to == PlaybackState::Idle || to == PlaybackState::Stopping;
        case PlaybackState::Error:
            return to == PlaybackState::Loading || to == PlaybackState::Idle || to == PlaybackState::Stopping;
        default:
            return false;
    }
}

} // namespace mnemis::playback
