#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <vector>

#include "playback/PlaybackEngine.hpp"

namespace mnemis::ui::controllers {

class PlaybackController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(double position READ position WRITE seek NOTIFY positionChanged)
    Q_PROPERTY(double duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(double playbackRate READ playbackRate WRITE setPlaybackRate NOTIFY playbackRateChanged)
    Q_PROPERTY(bool isBuffering READ isBuffering NOTIFY bufferingChanged)
    Q_PROPERTY(QString currentMedia READ currentMedia NOTIFY currentMediaChanged)
    Q_PROPERTY(QVariantList audioTracks READ audioTracks NOTIFY tracksChanged)
    Q_PROPERTY(QVariantList subtitleTracks READ subtitleTracks NOTIFY tracksChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    explicit PlaybackController(std::unique_ptr<playback::PlaybackEngine> engine, QObject* parent = nullptr);
    ~PlaybackController() override;

    QString state() const;
    double position() const;
    double duration() const;
    double volume() const;
    double playbackRate() const;
    bool isBuffering() const;
    QString currentMedia() const;
    QString error() const;
    
    QVariantList audioTracks() const;
    QVariantList subtitleTracks() const;

    Q_INVOKABLE void load(const QString& uri, const QString& origin = "User");
    Q_INVOKABLE void play(const QString& origin = "User");
    Q_INVOKABLE void pause(const QString& origin = "User");
    Q_INVOKABLE void stop(const QString& origin = "User");
    
    void seek(double positionSecs);
    void setVolume(double value);
    void setPlaybackRate(double rate);

signals:
    void stateChanged();
    void positionChanged();
    void durationChanged();
    void volumeChanged();
    void playbackRateChanged();
    void bufferingChanged();
    void currentMediaChanged();
    void tracksChanged();
    void errorChanged();
    
private:
    std::unique_ptr<playback::PlaybackEngine> m_engine;
    
    // Cached state for QML
    QString m_stateStr = "Idle";
    double m_position = 0.0;
    double m_duration = 0.0;
    double m_volume = 1.0;
    double m_playbackRate = 1.0;
    bool m_isBuffering = false;
    QString m_currentMedia;
    QString m_error;
    
    QVariantList m_audioTracks;
    QVariantList m_subtitleTracks;

    void updateState(playback::PlaybackState::Value state);
    playback::PlaybackOrigin::Value parseOrigin(const QString& originStr) const;
    
    static QVariantList mapTracks(const std::vector<playback::TrackInfo>& tracks);
};

} // namespace mnemis::ui::controllers
