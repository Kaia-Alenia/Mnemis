#pragma once

#include <QObject>

namespace mnemis::playback {

class PlaybackOrigin {
    Q_GADGET
public:
    enum Value {
        User,
        Plugin,
        System,
        Restore,
        Internal
    };
    Q_ENUM(Value)
};

class PlaybackState {
    Q_GADGET
public:
    enum Value {
        Idle,
        Loading,
        Ready,
        Playing,
        Paused,
        Seeking,
        Buffering,
        Stopping,
        Ended,
        Error
    };
    Q_ENUM(Value)
};

} // namespace mnemis::playback
