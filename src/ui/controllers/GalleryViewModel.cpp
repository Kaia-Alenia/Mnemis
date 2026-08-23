#include "GalleryViewModel.hpp"
#include <QThreadPool>
#include <QMetaObject>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <thread>
#include <cmath>

namespace mnemis::ui::controllers {

GalleryViewModel::GalleryViewModel(
    core::repositories::IMediaRepository* repository,
    QObject* parent)
    : QAbstractListModel(parent)
    , m_repository(repository)
    , m_isAlive(std::make_shared<bool>(true))
{
    reload();
}

GalleryViewModel::~GalleryViewModel() {
    *m_isAlive = false;
    if (m_eventBus && m_eventSubToken) {
        m_eventBus->unsubscribe(m_eventSubToken);
    }
    if (m_thumbnailEngine) {
        m_thumbnailEngine->clearPendingRequests();
    }
}

void GalleryViewModel::setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus) {
    if (m_eventBus && m_eventSubToken) {
        m_eventBus->unsubscribe(m_eventSubToken);
    }
    m_eventBus = std::move(eventBus);
    if (m_eventBus) {
        auto alive = m_isAlive;
        m_eventSubToken = m_eventBus->subscribe([this, alive](const core::events::LibraryEvent& event) {
            if (!*alive) return;
            // Capture event by value or copy fields since we post to UI thread
            auto self = const_cast<GalleryViewModel*>(this);
            QMetaObject::invokeMethod(self, [self, event]() {
                self->onLibraryEvent(event);
            }, Qt::QueuedConnection);
        });
    }
}

void GalleryViewModel::setThumbnailEngine(core::thumbnails::IThumbnailEngine* engine) {
    m_thumbnailEngine = engine;
}

void GalleryViewModel::setMediaListContext(MediaListContext* context) {
    m_listContext = context;
    if (m_listContext) {
        m_listContext->setQueryOptions(m_currentQuery);
    }
}

QHash<int, QByteArray> GalleryViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[MediaIdRole] = "mediaId";
    roles[FileNameRole] = "fileName";
    roles[CanonicalPathRole] = "canonicalPath";
    roles[MimeTypeRole] = "mimeType";
    roles[MediaTypeRole] = "mediaType";
    roles[FileSizeRole] = "fileSize";
    roles[ModifiedTimeRole] = "modifiedTime";
    roles[DurationRole] = "duration";
    roles[ThumbnailUrlRole] = "thumbnailUrl";
    roles[IsLoadedRole] = "isLoaded";
    roles[IsSelectedRole] = "isSelected";
    roles[ThumbnailStateRole] = "thumbnailState";
    return roles;
}

void GalleryViewModel::reload() {
    qInfo() << "GalleryViewModel: Reloading gallery data...";
    beginResetModel();
    m_chunks.clear();
    m_pendingChunks.clear();
    m_totalCount = 0;
    endResetModel();

    int currentGen = ++m_queryGeneration;

    auto alive = m_isAlive;
    auto repo = m_repository;
    auto query = m_currentQuery;

    QThreadPool::globalInstance()->start([this, alive, repo, query, currentGen]() {
        auto result = repo->count(query);
        if (!*alive) return;

        int count = result.isSuccess() ? result.value() : 0;

        auto self = const_cast<GalleryViewModel*>(this);
        QMetaObject::invokeMethod(self, [self, currentGen, count]() {
            self->onCountLoaded(currentGen, count);
        }, Qt::QueuedConnection);
    });
}

void GalleryViewModel::onCountLoaded(int generation, int count) {
    if (generation != m_queryGeneration) return; // Stale result

    qInfo() << "GalleryViewModel: Loaded total count:" << count;
    // QML needs a structural model notification when the asynchronous count
    // changes from the empty loading state to the result set. countChanged
    // alone only updates the sidebar label; it does not create GridView delegates.
    beginResetModel();
    m_totalCount = count;
    endResetModel();
    emit countChanged();
}

int GalleryViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_totalCount;
}

QVariant GalleryViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_totalCount)
        return {};

    int row = index.row();
    int chunkIndex = row / CHUNK_SIZE;
    int localIndex = row % CHUNK_SIZE;

    if (!m_chunks.contains(chunkIndex)) {
        requestChunk(chunkIndex, m_queryGeneration);
        if (role == IsLoadedRole) return false;
        return {};
    }

    const auto& chunk = m_chunks[chunkIndex];
    if (!chunk.isLoaded) {
        if (role == IsLoadedRole) return false;
        return {};
    }

    if (localIndex >= static_cast<int>(chunk.items.size())) {
        return {};
    }

    const auto& item = chunk.items[localIndex];

    switch (role) {
        case MediaIdRole: return QString::fromStdString(item.mediaId);
        case FileNameRole: return QString::fromStdString(item.fileName);
        case CanonicalPathRole: return QString::fromStdString(item.canonicalPath);
        case MimeTypeRole: return QString::fromStdString(item.mimeType);
        case MediaTypeRole: return static_cast<int>(item.mediaType);
        case FileSizeRole: return QVariant::fromValue(static_cast<qulonglong>(item.fileSize));
        case ModifiedTimeRole: return QVariant::fromValue(static_cast<qint64>(item.modifiedTime));
        case DurationRole: return item.duration.has_value() ? item.duration.value() : QVariant();
        case IsLoadedRole: return true;
        case IsSelectedRole: return isSelected(QString::fromStdString(item.mediaId));
        case ThumbnailStateRole: {
            QString qId = QString::fromStdString(item.mediaId);
            if (!m_thumbnailStates.contains(qId)) {
                m_thumbnailStates[qId] = core::thumbnails::ThumbnailStatus::Pending;
                requestThumbnail(qId, item);
            }
            return static_cast<int>(m_thumbnailStates[qId]);
        }
        case ThumbnailUrlRole: {
            QUrl url;
            url.setScheme("image");
            url.setHost("async_thumbnails");
            url.setPath("/" + QString::fromStdString(item.mediaId));

            QUrlQuery query;
            query.addQueryItem("path", QString::fromStdString(item.canonicalPath));
            query.addQueryItem("size", QString::number(item.fileSize));
            query.addQueryItem("time", QString::number(item.modifiedTime));
            url.setQuery(query);
            return url.toString();
        }
        default: return {};
    }
}

void GalleryViewModel::requestChunk(int chunkIndex, int generation) const {
    if (m_pendingChunks.contains(chunkIndex)) return;

    if (m_chunks.size() >= MAX_CACHED_CHUNKS) {
        int maxDist = -1;
        int chunkToEvict = -1;
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it) {
            int dist = std::abs(it.key() - chunkIndex);
            if (dist > maxDist) {
                maxDist = dist;
                chunkToEvict = it.key();
            }
        }
        if (chunkToEvict != -1 && maxDist > 1) { // keep nearby
            if (m_thumbnailEngine) {
                for (const auto& item : m_chunks[chunkToEvict].items) {
                    QString qId = QString::fromStdString(item.mediaId);
                    if (m_thumbnailTasks.contains(qId)) {
                        m_thumbnailEngine->cancelRequest(m_thumbnailTasks[qId]);
                        m_thumbnailTasks.remove(qId);
                        m_thumbnailStates.remove(qId);
                    }
                }
            }
            m_chunks.remove(chunkToEvict);
        }
    }

    qInfo() << "GalleryViewModel: Requesting chunk" << chunkIndex << "(generation" << generation << ")";
    m_pendingChunks.insert(chunkIndex);

    auto alive = m_isAlive;
    auto repo = m_repository;
    auto query = m_currentQuery;

    QThreadPool::globalInstance()->start([this, alive, repo, query, chunkIndex, generation]() {
        auto result = repo->list(chunkIndex + 1, CHUNK_SIZE, query); // SQLiteMediaRepository uses 1-based page
        if (!*alive) return;

        std::vector<core::models::MediaItem> items;
        if (result.isSuccess()) {
            items = std::move(result.value());
        }

        auto self = const_cast<GalleryViewModel*>(this);
        QMetaObject::invokeMethod(self, [self, chunkIndex, generation, items = std::move(items)]() mutable {
            self->onChunkLoaded(chunkIndex, generation, std::move(items));
        }, Qt::QueuedConnection);
    });
}

void GalleryViewModel::onChunkLoaded(int chunkIndex, int generation, std::vector<core::models::MediaItem> items) {
    if (generation != m_queryGeneration) return; // Stale result

    m_pendingChunks.remove(chunkIndex);

    int itemsSize = items.size();
    Chunk chunk;
    chunk.index = chunkIndex;
    chunk.items = std::move(items);
    chunk.isLoaded = true;
    m_chunks[chunkIndex] = std::move(chunk);

    int startIndex = chunkIndex * CHUNK_SIZE;
    int endIndex = startIndex + itemsSize - 1;
    if (endIndex >= startIndex) {
        emit dataChanged(index(startIndex, 0), index(endIndex, 0));
    }
}

void GalleryViewModel::setSortOptions(const QString& sortBy, bool ascending) {
    qInfo() << "GalleryViewModel: Sort changed:" << sortBy << "ascending=" << ascending;
    m_currentQuery.sortBy = sortBy.toStdString();
    m_currentQuery.ascending = ascending;
    if (m_listContext) m_listContext->setQueryOptions(m_currentQuery);
    reload();
}

void GalleryViewModel::setFilter(const QString& filterText, int filterMediaType) {
    qInfo() << "GalleryViewModel: Filter changed: text=" << filterText
            << "mediaType=" << filterMediaType;
    if (filterText.isEmpty()) {
        m_currentQuery.filterText = std::nullopt;
    } else {
        m_currentQuery.filterText = filterText.toStdString();
    }

    if (filterMediaType < 0) {
        m_currentQuery.filterMediaType = std::nullopt;
    } else {
        m_currentQuery.filterMediaType = static_cast<core::models::MediaType>(filterMediaType);
    }
    m_currentQuery.filterFavorite = std::nullopt;
    if (!m_selectedIds.empty()) {
        m_selectedIds.clear();
        emit selectionChanged();
    }
    if (m_listContext) m_listContext->setQueryOptions(m_currentQuery);
    reload();
}

void GalleryViewModel::setFavoriteFilter(bool onlyFavorites, const QString& filterText) {
    qInfo() << "GalleryViewModel: Favorite filter changed:" << onlyFavorites
            << "text=" << filterText;
    if (filterText.isEmpty()) {
        m_currentQuery.filterText = std::nullopt;
    } else {
        m_currentQuery.filterText = filterText.toStdString();
    }
    m_currentQuery.filterFavorite = onlyFavorites ? std::optional<bool>(true) : std::nullopt;
    m_currentQuery.filterMediaType = std::nullopt;
    if (!m_selectedIds.empty()) {
        m_selectedIds.clear();
        emit selectionChanged();
    }
    if (m_listContext) m_listContext->setQueryOptions(m_currentQuery);
    reload();
}

void GalleryViewModel::toggleSelection(const QString& mediaId) {
    std::string id = mediaId.toStdString();
    if (m_selectedIds.find(id) != m_selectedIds.end()) {
        m_selectedIds.erase(id);
        qInfo() << "GalleryViewModel: Deselected" << mediaId;
    } else {
        m_selectedIds.insert(id);
        qInfo() << "GalleryViewModel: Selected (multi)" << mediaId;
    }
    emit selectionChanged();

    // Notify the model that a specific item's IsSelectedRole changed
    // Since we don't map ID to row directly, we might need to search or emit generic update.
    // For large lists, a generic update is slow. Let's find the row.
    for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it) {
        int chunkIdx = it.key();
        const auto& items = it.value().items;
        for (size_t i = 0; i < items.size(); ++i) {
            if (items[i].mediaId == id) {
                int row = chunkIdx * CHUNK_SIZE + i;
                emit dataChanged(index(row, 0), index(row, 0), {IsSelectedRole});
                return;
            }
        }
    }
}

void GalleryViewModel::selectOne(const QString& mediaId) {
    qInfo() << "GalleryViewModel: Selected (single)" << mediaId;
    std::string id = mediaId.toStdString();
    if (m_selectedIds.size() == 1 && m_selectedIds.find(id) != m_selectedIds.end()) {
        return;
    }
    m_selectedIds.clear();
    m_selectedIds.insert(id);
    emit selectionChanged();

    for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it) {
        int chunkIdx = it.key();
        int startIndex = chunkIdx * CHUNK_SIZE;
        emit dataChanged(index(startIndex, 0), index(startIndex + it.value().items.size() - 1, 0), {IsSelectedRole});
    }
}
void GalleryViewModel::clearSelection() {
    qInfo() << "GalleryViewModel: Clearing selection; count=" << m_selectedIds.size();
    if (m_selectedIds.empty()) return;

    // Save previous to know what to update, or just update all loaded chunks
    m_selectedIds.clear();
    emit selectionChanged();

    for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it) {
        int chunkIdx = it.key();
        const auto& items = it.value().items;
        int startIndex = chunkIdx * CHUNK_SIZE;
        emit dataChanged(index(startIndex, 0), index(startIndex + items.size() - 1, 0), {IsSelectedRole});
    }
}

void GalleryViewModel::selectAll() {
    // This requires loading all IDs which is exactly what we want to avoid with virtualization.
    // However, if we must select all, we'd need a repository query to get all IDs.
    // For now, let's defer full selectAll if it breaks virtualization rules, or just select currently loaded.
    // Wait, the prompt says "Implementar toggleSelection(mediaId), clearSelection(), selectAll()".
    // Let's implement it by querying all IDs synchronously or asynchronously.
    auto repo = m_repository;
    auto query = m_currentQuery;
    auto alive = m_isAlive;

    std::thread([this, alive, repo, query]() {
        // We can request list with a large pageSize or repeatedly to gather IDs
        // SQLite is fast enough for getting just IDs, but our repo interface `list()` returns full MediaItems.
        // We could add `listIds()` or just live with the overhead for now as requested.
        int pageSize = 10000;
        int page = 1;
        std::vector<std::string> allIds;

        while (true) {
            auto res = repo->list(page, pageSize, query);
            if (!*alive || !res.isSuccess() || res.value().empty()) break;
            for (const auto& item : res.value()) {
                allIds.push_back(item.mediaId);
            }
            if (res.value().size() < pageSize) break;
            page++;
        }

        if (!*alive) return;

        auto self = const_cast<GalleryViewModel*>(this);
        QMetaObject::invokeMethod(self, [self, ids = std::move(allIds)]() {
            self->m_selectedIds.insert(ids.begin(), ids.end());
            emit self->selectionChanged();

            // Notify currently loaded
            for (auto it = self->m_chunks.begin(); it != self->m_chunks.end(); ++it) {
                int chunkIdx = it.key();
                int startIndex = chunkIdx * CHUNK_SIZE;
                emit self->dataChanged(self->index(startIndex, 0), self->index(startIndex + it.value().items.size() - 1, 0), {IsSelectedRole});
            }
        }, Qt::QueuedConnection);
    }).detach();
}

bool GalleryViewModel::isSelected(const QString& mediaId) const {
    return m_selectedIds.find(mediaId.toStdString()) != m_selectedIds.end();
}

QStringList GalleryViewModel::getSelectedMediaIds() const {
    QStringList result;
    for (const auto& id : m_selectedIds) {
        result << QString::fromStdString(id);
    }
    return result;
}

void GalleryViewModel::onLibraryEvent(const core::events::LibraryEvent& event) {
    using namespace core::events;
    if (event.type == LibraryEvent::Type::Updated) {
        QString qId = QString::fromStdString(event.mediaId);
        // Find the item and emit dataChanged
        for (auto it = m_chunks.begin(); it != m_chunks.end(); ++it) {
            const auto& items = it.value().items;
            for (size_t i = 0; i < items.size(); ++i) {
                if (QString::fromStdString(items[i].mediaId) == qId) {
                    int row = it.key() * CHUNK_SIZE + i;
                    // For now, if updated, maybe we invalidate its thumbnail state
                    m_thumbnailStates.remove(qId);
                    // Just emit dataChanged for all roles
                    emit dataChanged(index(row, 0), index(row, 0));
                    return;
                }
            }
        }
    } else {
        // Added, Removed, BatchAdded, BatchRemoved -> Reload
        qInfo() << "GalleryViewModel: Structural library event received, invalidating cache.";
        invalidateCache();
    }
}

void GalleryViewModel::invalidateCache() {
    // A structural change means chunks might be invalid, so we reload.
    // m_selectedIds are kept so selection is stable.
    m_thumbnailStates.clear();
    m_thumbnailTasks.clear();
    if (m_thumbnailEngine) {
        m_thumbnailEngine->clearPendingRequests();
    }
    reload();
}

void GalleryViewModel::requestThumbnail(const QString& mediaId, const core::models::MediaItem& item) const {
    if (!m_thumbnailEngine) return;

    core::thumbnails::ThumbnailSpec spec;
    spec.mediaId = item.mediaId;
    spec.canonicalPath = item.canonicalPath;
    spec.fileSize = item.fileSize;
    spec.modifiedTime = item.modifiedTime;
    spec.targetWidth = 256;
    spec.targetHeight = 256;
    spec.version = 1;

    auto alive = m_isAlive;
    auto self = const_cast<GalleryViewModel*>(this);

    core::thumbnails::ThumbnailTaskId taskId = m_thumbnailEngine->requestThumbnail(
        spec,
        core::thumbnails::ThumbnailPriority::Visible,
        [alive, self, qId = mediaId](const core::thumbnails::ThumbnailResult& res) {
            if (!*alive) return;
            QMetaObject::invokeMethod(self, [self, qId, status = res.status]() {
                self->m_thumbnailStates[qId] = status;

                // Find row to emit dataChanged
                for (auto it = self->m_chunks.begin(); it != self->m_chunks.end(); ++it) {
                    const auto& items = it.value().items;
                    for (size_t i = 0; i < items.size(); ++i) {
                        if (QString::fromStdString(items[i].mediaId) == qId) {
                            int row = it.key() * GalleryViewModel::CHUNK_SIZE + i;
                            emit self->dataChanged(self->index(row, 0), self->index(row, 0), {ThumbnailStateRole});
                            return;
                        }
                    }
                }
            }, Qt::QueuedConnection);
        }
    );

    m_thumbnailTasks[mediaId] = taskId;
}

} // namespace mnemis::ui::controllers
