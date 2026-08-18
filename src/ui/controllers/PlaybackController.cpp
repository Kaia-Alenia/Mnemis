#include "PlaybackController.hpp"
#include <QDebug>

namespace mnemis::ui::controllers {

PlaybackController::PlaybackController(std::unique_ptr<playback::PlaybackEngine> engine, QObject* parent)
    : QObject(parent)
    , m_engine(std::move(engine))
{
    if (m_engine) {
        // Wire up engine signals/callbacks to controller updates
        m_engine->setStateChangedCallback([this](playback::PlaybackState::Value newState) {
            updateState(newState);
        });

        m_engine->setPositionChangedCallback([this](double pos) {
            if (!qFuzzyCompare(m_position, pos)) {
                m_position = pos;
                emit positionChanged();
            }
        });

        m_engine->setDurationChangedCallback([this](double dur) {
            if (!qFuzzyCompare(m_duration, dur)) {
                m_duration = dur;
                emit durationChanged();
            }
        });

        m_engine->setBufferingChangedCallback([this](bool buffering) {
            if (m_isBuffering != buffering) {
                m_isBuffering = buffering;
                emit bufferingChanged();
            }
        });

        m_engine->setErrorCallback([this](const std::string& err) {
            m_error = QString::fromStdString(err);
            emit errorChanged();
        });

        m_engine->setTracksChangedCallback([this]() {
            m_audioTracks = mapTracks(m_engine->audioTracks());
            m_subtitleTracks = mapTracks(m_engine->subtitleTracks());
            emit tracksChanged();
        });

        // Initialize state
        updateState(m_engine->state());
    }
}

PlaybackController::~PlaybackController() = default;

QString PlaybackController::state() const { return m_stateStr; }
double PlaybackController::position() const { return m_position; }
double PlaybackController::duration() const { return m_duration; }
double PlaybackController::volume() const { return m_volume; }
double PlaybackController::playbackRate() const { return m_playbackRate; }
bool PlaybackController::isBuffering() const { return m_isBuffering; }
QString PlaybackController::currentMedia() const { return m_currentMedia; }
QString PlaybackController::error() const { return m_error; }

void* PlaybackController::getNativePlayer() const {
    if (m_engine) {
        return m_engine->getNativePlayer();
    }
    return nullptr;
}

QVariantList PlaybackController::audioTracks() const { return m_audioTracks; }
QVariantList PlaybackController::subtitleTracks() const { return m_subtitleTracks; }

void PlaybackController::load(const QString& uri, const QString& origin) {
    if (!m_engine) return;
    m_currentMedia = uri;
    emit currentMediaChanged();
    
    // Reset previous state
    m_error.clear();
    emit errorChanged();
    
    m_engine->load(uri.toStdString(), parseOrigin(origin));
}

void PlaybackController::play(const QString& origin) {
    if (m_engine) {
        m_engine->play(parseOrigin(origin));
    }
}

void PlaybackController::pause(const QString& origin) {
    if (m_engine) {
        m_engine->pause(parseOrigin(origin));
    }
}

void PlaybackController::stop(const QString& origin) {
    if (m_engine) {
        m_engine->stop(parseOrigin(origin));
        m_currentMedia.clear();
        emit currentMediaChanged();
    }
}

void PlaybackController::seek(double positionSecs) {
    if (m_engine) {
        m_engine->seek(positionSecs, playback::PlaybackOrigin::User);
    }
}

void PlaybackController::setVolume(double value) {
    if (m_engine && !qFuzzyCompare(m_volume, value)) {
        m_volume = value;
        m_engine->setVolume(value);
        emit volumeChanged();
    }
}

void PlaybackController::setPlaybackRate(double rate) {
    if (m_engine && !qFuzzyCompare(m_playbackRate, rate)) {
        m_playbackRate = rate;
        m_engine->setPlaybackRate(rate);
        emit playbackRateChanged();
    }
}

void PlaybackController::updateState(playback::PlaybackState::Value state) {
    QString newStr;
    switch (state) {
        case playback::PlaybackState::Idle: newStr = "Idle"; break;
        case playback::PlaybackState::Loading: newStr = "Loading"; break;
        case playback::PlaybackState::Ready: newStr = "Ready"; break;
        case playback::PlaybackState::Playing: newStr = "Playing"; break;
        case playback::PlaybackState::Paused: newStr = "Paused"; break;
        case playback::PlaybackState::Buffering: newStr = "Buffering"; break;
        case playback::PlaybackState::Ended: newStr = "Ended"; break;
        case playback::PlaybackState::Error: newStr = "Error"; break;
        default: newStr = "Idle"; break;
    }
    
    if (m_stateStr != newStr) {
        m_stateStr = newStr;
        emit stateChanged();
    }
}

playback::PlaybackOrigin::Value PlaybackController::parseOrigin(const QString& originStr) const {
    if (originStr == "System") return playback::PlaybackOrigin::System;
    if (originStr == "Plugin") return playback::PlaybackOrigin::Plugin;
    return playback::PlaybackOrigin::User;
}

QVariantList PlaybackController::mapTracks(const std::vector<playback::TrackInfo>& tracks) {
    QVariantList list;
    for (const auto& track : tracks) {
        QVariantMap map;
        for (const auto& [key, value] : track) {
            if (std::holds_alternative<int>(value)) {
                map[QString::fromStdString(key)] = std::get<int>(value);
            } else if (std::holds_alternative<std::string>(value)) {
                map[QString::fromStdString(key)] = QString::fromStdString(std::get<std::string>(value));
            } else if (std::holds_alternative<bool>(value)) {
                map[QString::fromStdString(key)] = std::get<bool>(value);
            }
        }
        list.append(map);
    }
    return list;
}

} // namespace mnemis::ui::controllers
