#pragma once

#include "playback/IPlaybackBackend.hpp"
#include "core/ILogger.hpp"
#include <mpv/client.h>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>
#include <vector>

namespace mnemis::playback {

class LibMpvBackend : public IPlaybackBackend {
public:
    explicit LibMpvBackend(core::ILogger& logger);
    ~LibMpvBackend() override;

    void setDelegate(IPlaybackBackendDelegate* delegate) override;
    
    void load(const std::string& uri) override;
    void play() override;
    void pause() override;
    void stop() override;
    void seek(double positionSecs) override;
    void setVolume(double value) override;
    void setPlaybackRate(double rate) override;
    void* getNativePlayer() const override;

    // Call this repeatedly from a UI event loop or timer to process events
    // In a real Qt app, we would use mpv_set_wakeup_callback and QMetaObject::invokeMethod.
    void processEvents();

private:
    void handleMpvEvent(mpv_event* event);
    void handlePropertyChange(mpv_event_property* prop);
    void updateTracks();

    core::ILogger& m_logger;
    mpv_handle* m_mpv = nullptr;
    IPlaybackBackendDelegate* m_delegate = nullptr;
    std::mutex m_mutex;
    
    // Qt integration point
    static void onMpvWakeup(void* ctx);
};

} // namespace mnemis::playback
