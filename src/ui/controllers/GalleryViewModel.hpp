#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QSet>
#include <memory>
#include <vector>
#include <unordered_set>
#include <atomic>
#include <thread>

#include "MediaListContext.hpp"
#include "core/models/MediaItem.hpp"
#include "core/repositories/IMediaRepository.hpp"
#include "core/thumbnails/IThumbnailEngine.hpp"
#include "core/events/ILibraryEventBus.hpp"

namespace mnemis::ui::controllers {

class GalleryViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        MediaIdRole = Qt::UserRole + 1,
        FileNameRole,
        CanonicalPathRole,
        MimeTypeRole,
        MediaTypeRole,
        FileSizeRole,
        ModifiedTimeRole,
        DurationRole,
        ThumbnailUrlRole,
        IsLoadedRole,
        IsSelectedRole,
        ThumbnailStateRole
    };

    explicit GalleryViewModel(
        core::repositories::IMediaRepository* repository,
        QObject* parent = nullptr);

    ~GalleryViewModel() override;

    void setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus);
    void setThumbnailEngine(core::thumbnails::IThumbnailEngine* engine);
    void setMediaListContext(MediaListContext* context);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void reload();
    void setSortOptions(const QString& sortBy, bool ascending);
    void setFilter(const QString& filterText, int filterMediaType = -1);
    void setFavoriteFilter(bool onlyFavorites, const QString& filterText = {});
    void setRecentFilter(bool onlyRecent, const QString& filterText = {});

    Q_INVOKABLE void toggleSelection(const QString& mediaId);
    Q_INVOKABLE void selectOne(const QString& mediaId);
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE bool isSelected(const QString& mediaId) const;
    Q_INVOKABLE QStringList getSelectedMediaIds() const;

signals:
    void countChanged();
    void selectionChanged();

private slots:
    void onLibraryEvent(const core::events::LibraryEvent& event);

private:
    struct Chunk {
        int index;
        std::vector<core::models::MediaItem> items;
        bool isLoaded = false;
    };

    void requestChunk(int chunkIndex, int generation) const;
    Q_INVOKABLE void onChunkLoaded(int chunkIndex, int generation, std::vector<core::models::MediaItem> items);
    Q_INVOKABLE void onCountLoaded(int generation, int count);
    void invalidateCache();
    void requestThumbnail(const QString& mediaId, const core::models::MediaItem& item) const;

    core::repositories::IMediaRepository* m_repository;
    int m_totalCount = 0;
    
    static constexpr int CHUNK_SIZE = 100;
    static constexpr int MAX_CACHED_CHUNKS = 10;
    
    mutable QHash<int, Chunk> m_chunks;
    mutable QSet<int> m_pendingChunks;
    
    std::unordered_set<std::string> m_selectedIds;
    core::repositories::QueryOptions m_currentQuery;
    std::atomic<int> m_queryGeneration{0};
    
    std::shared_ptr<core::events::ILibraryEventBus> m_eventBus;
    core::events::ILibraryEventBus::SubscriptionToken m_eventSubToken = 0;
    
    core::thumbnails::IThumbnailEngine* m_thumbnailEngine = nullptr;
    mutable QHash<QString, core::thumbnails::ThumbnailStatus> m_thumbnailStates;
    mutable QHash<QString, core::thumbnails::ThumbnailTaskId> m_thumbnailTasks;

    MediaListContext* m_listContext = nullptr;

    std::shared_ptr<bool> m_isAlive;
    std::thread m_selectAllThread;
};

} // namespace mnemis::ui::controllers
