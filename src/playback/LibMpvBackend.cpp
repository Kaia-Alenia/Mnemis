#include "playback/LibMpvBackend.hpp"
#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>
#include <iostream>

namespace mnemis::playback {

LibMpvBackend::LibMpvBackend(core::ILogger& logger)
    : m_logger(logger) {
    m_mpv = mpv_create();
    if (!m_mpv) {
        m_logger.log(core::LogLevel::Error, "Failed to create mpv context");
    } else {
        // Core rendering and input options
        mpv_set_option_string(m_mpv, "hwdec", "no");
        mpv_set_option_string(m_mpv, "vo", "libmpv");
        mpv_set_option_string(m_mpv, "input-default-bindings", "no");
        mpv_set_option_string(m_mpv, "input-vo-keyboard", "no");
        // Start paused so PlaybackEngine controls when playback begins
        mpv_set_option_string(m_mpv, "pause", "yes");
        // Keep demuxer cache generous so files >3s don't stall
        mpv_set_option_string(m_mpv, "demuxer-max-bytes", "50MiB");
        mpv_set_option_string(m_mpv, "demuxer-max-back-bytes", "10MiB");
        mpv_set_option_string(m_mpv, "cache", "yes");
        
        int rc = mpv_initialize(m_mpv);
        if (rc < 0) {
            m_logger.log(core::LogLevel::Error, "Failed to initialize mpv");
        } else {
            m_logger.log(core::LogLevel::Info, "MPV context initialized");
            
            // Observe properties
            mpv_observe_property(m_mpv, 0, "time-pos", MPV_FORMAT_DOUBLE);
            mpv_observe_property(m_mpv, 0, "duration", MPV_FORMAT_DOUBLE);
            mpv_observe_property(m_mpv, 0, "pause", MPV_FORMAT_FLAG);
            mpv_observe_property(m_mpv, 0, "core-idle", MPV_FORMAT_FLAG);
            mpv_observe_property(m_mpv, 0, "eof-reached", MPV_FORMAT_FLAG);
            mpv_observe_property(m_mpv, 0, "track-list", MPV_FORMAT_NODE);
            mpv_observe_property(m_mpv, 0, "paused-for-cache", MPV_FORMAT_FLAG);
            // "warn" level: only real errors and warnings, no FFmpeg debug spam
            mpv_request_log_messages(m_mpv, "warn");
            mpv_set_wakeup_callback(m_mpv, onMpvWakeup, this);
        }
    }
}

void LibMpvBackend::onMpvWakeup(void* ctx) {
    auto self = static_cast<LibMpvBackend*>(ctx);
    if (QCoreApplication::instance()) {
        QMetaObject::invokeMethod(QCoreApplication::instance(), [self]() {
            self->processEvents();
        }, Qt::QueuedConnection);
    }
}

LibMpvBackend::~LibMpvBackend() {
    if (m_mpv) {
        mpv_terminate_destroy(m_mpv);
        m_mpv = nullptr;
        m_logger.log(core::LogLevel::Info, "MPV context destroyed");
    }
}

void LibMpvBackend::setDelegate(IPlaybackBackendDelegate* delegate) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_delegate = delegate;
}

void LibMpvBackend::load(const std::string& uri) {
    if (!m_mpv) return;
    // Pass "pause=no" as a per-file option so MPV starts playing immediately
    // when the file is loaded, regardless of the global pause state.
    // This eliminates the race condition between load() and play().
    const char* args[] = {"loadfile", uri.c_str(), "replace", "pause=no", NULL};
    mpv_command_async(m_mpv, 0, args);
    m_logger.log(core::LogLevel::Info, "MPV loadfile: " + uri);
}

void LibMpvBackend::play() {
    if (!m_mpv) return;
    int pause = 0;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &pause);
}

void LibMpvBackend::pause() {
    if (!m_mpv) return;
    int pause = 1;
    mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &pause);
}

void LibMpvBackend::stop() {
    if (!m_mpv) return;
    const char* args[] = {"stop", NULL};
    mpv_command_async(m_mpv, 0, args);
}

void LibMpvBackend::seek(double positionSecs) {
    if (!m_mpv) return;
    std::string posStr = std::to_string(positionSecs);
    const char* args[] = {"seek", posStr.c_str(), "absolute", NULL};
    mpv_command_async(m_mpv, 0, args);
}

void LibMpvBackend::setVolume(double value) {
    if (!m_mpv) return;
    double vol = value * 100.0; // mpv uses 0-100 or higher
    mpv_set_property_async(m_mpv, 0, "volume", MPV_FORMAT_DOUBLE, &vol);
}

void LibMpvBackend::setPlaybackRate(double rate) {
    if (!m_mpv) return;
    mpv_set_property_async(m_mpv, 0, "speed", MPV_FORMAT_DOUBLE, &rate);
}

void LibMpvBackend::setAudioTrack(int id) {
    if (!m_mpv) return;
    if (id < 0) {
        const char* val = "no";
        mpv_set_property_async(m_mpv, 0, "aid", MPV_FORMAT_STRING, &val);
    } else {
        mpv_set_property_async(m_mpv, 0, "aid", MPV_FORMAT_INT64, &id);
    }
}

void LibMpvBackend::setSubtitleTrack(int id) {
    if (!m_mpv) return;
    if (id < 0) {
        const char* val = "no";
        mpv_set_property_async(m_mpv, 0, "sid", MPV_FORMAT_STRING, &val);
    } else {
        mpv_set_property_async(m_mpv, 0, "sid", MPV_FORMAT_INT64, &id);
    }
}

void LibMpvBackend::processEvents() {
    if (!m_mpv) return;
    while (m_mpv) {
        mpv_event* event = mpv_wait_event(m_mpv, 0);
        if (event->event_id == MPV_EVENT_NONE) {
            break;
        }
        handleMpvEvent(event);
    }
}

void LibMpvBackend::handleMpvEvent(mpv_event* event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_delegate) return;

    switch (event->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE: {
            mpv_event_property* prop = (mpv_event_property*)event->data;
            handlePropertyChange(prop);
            break;
        }
        case MPV_EVENT_START_FILE:
            m_logger.log(core::LogLevel::Info, "[MPV] File loading started");
            m_delegate->onStateChanged(PlaybackState::Loading);
            break;
        case MPV_EVENT_FILE_LOADED:
            m_logger.log(core::LogLevel::Info, "[MPV] File loaded — triggering play");
            m_delegate->onStateChanged(PlaybackState::Ready);
            // Auto-start playback: unpause after file is fully loaded
            {
                int unpause = 0;
                mpv_set_property_async(m_mpv, 0, "pause", MPV_FORMAT_FLAG, &unpause);
            }
            break;
        case MPV_EVENT_END_FILE: {
            mpv_event_end_file* eef = (mpv_event_end_file*)event->data;
            if (eef->reason == MPV_END_FILE_REASON_EOF) {
                m_delegate->onEnded();
            } else if (eef->reason == MPV_END_FILE_REASON_ERROR) {
                m_delegate->onError(mpv_error_string(eef->error));
            }
            break;
        }
        case MPV_EVENT_SEEK:
            m_delegate->onStateChanged(PlaybackState::Seeking);
            break;
        case MPV_EVENT_LOG_MESSAGE: {
            mpv_event_log_message* msg = (mpv_event_log_message*)event->data;
            // Only surface errors and warnings to avoid flooding output
            if (msg->log_level <= MPV_LOG_LEVEL_WARN) {
                std::cerr << "[MPV] " << msg->prefix << ": " << msg->text;
            }
            break;
        }
        default:
            break;
    }
}

void LibMpvBackend::handlePropertyChange(mpv_event_property* prop) {
    std::string name(prop->name);
    
    if (name == "time-pos" && prop->format == MPV_FORMAT_DOUBLE) {
        m_delegate->onPositionChanged(*(double*)prop->data);
    } 
    else if (name == "duration" && prop->format == MPV_FORMAT_DOUBLE) {
        m_delegate->onDurationChanged(*(double*)prop->data);
    }
    else if (name == "pause" && prop->format == MPV_FORMAT_FLAG) {
        int paused = *(int*)prop->data;
        m_delegate->onStateChanged(paused ? PlaybackState::Paused : PlaybackState::Playing);
    }
    else if (name == "paused-for-cache" && prop->format == MPV_FORMAT_FLAG) {
        int buffering = *(int*)prop->data;
        m_delegate->onBufferingChanged(buffering != 0);
        if (buffering) {
            m_delegate->onStateChanged(PlaybackState::Buffering);
        }
    }
    else if (name == "track-list") {
        updateTracks();
    }
}

void LibMpvBackend::updateTracks() {
    if (!m_mpv || !m_delegate) return;
    
    mpv_node node;
    if (mpv_get_property(m_mpv, "track-list", MPV_FORMAT_NODE, &node) < 0) {
        return;
    }
    
    std::vector<TrackInfo> audioTracks;
    std::vector<TrackInfo> subtitleTracks;
    
    if (node.format == MPV_FORMAT_NODE_ARRAY) {
        mpv_node_list* list = node.u.list;
        for (int i = 0; i < list->num; i++) {
            if (list->values[i].format == MPV_FORMAT_NODE_MAP) {
                mpv_node_list* map = list->values[i].u.list;
                TrackInfo info;
                std::string type;
                
                for (int j = 0; j < map->num; j++) {
                    std::string key = map->keys[j];
                    if (key == "id" && map->values[j].format == MPV_FORMAT_INT64) {
                        info["id"] = static_cast<int>(map->values[j].u.int64);
                    } else if (key == "type" && map->values[j].format == MPV_FORMAT_STRING) {
                        type = map->values[j].u.string;
                        info["type"] = type;
                    } else if (key == "title" && map->values[j].format == MPV_FORMAT_STRING) {
                        info["title"] = std::string(map->values[j].u.string);
                    } else if (key == "lang" && map->values[j].format == MPV_FORMAT_STRING) {
                        info["language"] = std::string(map->values[j].u.string);
                    } else if (key == "codec" && map->values[j].format == MPV_FORMAT_STRING) {
                        info["codec"] = std::string(map->values[j].u.string);
                    } else if (key == "selected" && map->values[j].format == MPV_FORMAT_FLAG) {
                        info["selected"] = static_cast<bool>(map->values[j].u.flag);
                    }
                }
                
                if (type == "audio") {
                    audioTracks.push_back(info);
                } else if (type == "sub") {
                    // Normalize 'sub' to 'subtitle' as per user's contract mapping
                    info["type"] = std::string("subtitle");
                    subtitleTracks.push_back(info);
                }
            }
        }
    }
    
    mpv_free_node_contents(&node);
    
    m_delegate->onTracksChanged(audioTracks, subtitleTracks);
}

void* LibMpvBackend::getNativePlayer() const {
    return m_mpv;
}

} // namespace mnemis::playback
