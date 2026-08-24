#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include "core/repositories/IPlaylistRepository.hpp"
#include "core/ILogger.hpp"

namespace mnemis::ui::controllers {

class PlaylistController : public QObject {
    Q_OBJECT

public:
    explicit PlaylistController(core::repositories::IPlaylistRepository& repo, core::ILogger& logger, QObject* parent = nullptr);

    Q_INVOKABLE void loadPlaylists();
    Q_INVOKABLE void createPlaylist(const QString& name);
    Q_INVOKABLE void deletePlaylist(const QString& playlistId);
    Q_INVOKABLE void addMediaToPlaylist(const QString& playlistId, const QString& mediaId);
    Q_INVOKABLE void removeMediaFromPlaylist(const QString& playlistId, const QString& mediaId);

signals:
    void playlistsLoaded(const QVariantList& playlists);
    void errorOccurred(const QString& message);

private:
    core::repositories::IPlaylistRepository& m_repo;
    core::ILogger& m_logger;
};

} // namespace mnemis::ui::controllers
