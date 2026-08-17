# Mnemis Playback API Surface

This document provides a technical overview of the new Playback API surface introduced in Phase 14.

## Core Interfaces

### `IPlaybackBackend`
The backend abstraction interface. This interface isolates the core logic from the specific playback engine used (e.g., LibMPV).

- `virtual void load(const QString& path) = 0;`
- `virtual void play() = 0;`
- `virtual void pause() = 0;`
- `virtual void stop() = 0;`
- `virtual void seek(qint64 positionMs) = 0;`
- `virtual void setVolume(int volume) = 0;`
- `virtual void setMuted(bool muted) = 0;`

**Signals (via callback/listener or QObject signals in implementations):**
- Position changes
- Duration changes
- State changes (Playing, Paused, Stopped, Error)
- Error reporting

### `PlaybackEngine`
The central manager for playback state and operations. It proxies commands to the active backend and maintains a strict state machine (`Idle` -> `Loading` -> `Ready`).

**Thread Safety:**
Uses `std::recursive_mutex` to allow safe synchronous callbacks from backends (e.g., when a backend fires an event on the same thread during a state transition).

**Key Methods:**
- `void setBackend(std::unique_ptr<IPlaybackBackend> backend)`
- `void load(const QString& path)`
- `void play()`
- `void pause()`
- `void stop()`

**Key Properties / Signals:**
- `PlaybackState state() const`
- `qint64 position() const`
- `qint64 duration() const`
- `QString errorString() const`

## Implementations

### `LibMpvBackend`
The production backend using `libmpv`.

**Important Constraints:**
- **Locale:** Requires `LC_NUMERIC="C"`. `libmpv` is notoriously sensitive to locale settings.
- **Event Loop:** Exposes a wakeup callback mechanism to integrate the `mpv_wait_event` loop into Qt's event loop via `mpv_set_wakeup_cb`.

### `FakePlaybackBackend`
A testing dummy backend used in unit tests to simulate playback state changes without invoking `libmpv`. Supports synchronous event firing to validate `PlaybackEngine` re-entrancy and state machine constraints.

## PlaybackController
The QML-facing facade (if applicable, inheriting from `PlaybackEngine` or wrapping it) providing Q_PROPERTY bindings.
