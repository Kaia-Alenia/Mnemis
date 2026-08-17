#include "ViewerViewModel.hpp"
#include <QMetaObject>
#include <QFileInfo>
#include <thread>
#include <QThreadPool>
#include <variant>
#include "core/events/ILibraryEventBus.hpp"

namespace mnemis::ui::controllers {

ViewerViewModel::ViewerViewModel(
    core::repositories::IMediaRepository* repository,
    QObject* parent)
    : QObject(parent),
      m_repository(repository),
      m_animatedController(new AnimatedMediaController(this)),
      m_isAlive(std::make_shared<bool>(true))
{
}

ViewerViewModel::~ViewerViewModel() {
    if (m_eventBus && m_eventSubToken > 0) {
        m_eventBus->unsubscribe(m_eventSubToken);
    }
}

void ViewerViewModel::setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus) {
    if (m_eventBus && m_eventSubToken > 0) {
        m_eventBus->unsubscribe(m_eventSubToken);
    }
    m_eventBus = eventBus;
    if (m_eventBus) {
        m_eventSubToken = m_eventBus->subscribe([this](const core::events::LibraryEvent& event) {
            onLibraryEvent(event);
        });
    }
}

void ViewerViewModel::setContext(MediaListContext* context) {
    if (m_context) {
        disconnect(m_context, &MediaListContext::currentMediaIdChanged,
                   this, &ViewerViewModel::onContextMediaIdChanged);
    }
    
    m_context = context;
    
    if (m_context) {
        connect(m_context, &MediaListContext::currentMediaIdChanged,
                this, &ViewerViewModel::onContextMediaIdChanged);
        onContextMediaIdChanged(); // Load initial state
    }
}

QString ViewerViewModel::mediaId() const { return m_mediaId; }
QString ViewerViewModel::canonicalPath() const { return m_canonicalPath; }
QString ViewerViewModel::title() const { return m_title; }
qint64 ViewerViewModel::fileSize() const { return m_fileSize; }
QSize ViewerViewModel::resolution() const { return m_resolution; }
bool ViewerViewModel::isFavorite() const { return m_isFavorite; }
AnimatedMediaController* ViewerViewModel::animatedController() const { return m_animatedController; }

QString ViewerViewModel::imageState() const {
    switch (m_imageState) {
        case ImageState::Loading: return "Loading";
        case ImageState::Ready: return "Ready";
        case ImageState::Error: return "Error";
        case ImageState::Unavailable: return "Unavailable";
    }
    return "Unavailable";
}

int ViewerViewModel::rotation() const { return m_rotation; }

bool ViewerViewModel::isFullscreen() const { return m_isFullscreen; }

void ViewerViewModel::rotate(int degrees) {
    int newRotation = (m_rotation + degrees) % 360;
    if (newRotation < 0) {
        newRotation += 360;
    }
    
    if (m_rotation != newRotation) {
        m_rotation = newRotation;
        emit rotationChanged();
    }
}

void ViewerViewModel::toggleFavorite() {
    if (m_mediaId.isEmpty() || !m_repository) {
        return;
    }

    std::string id = m_mediaId.toStdString();
    bool newFavorite = !m_isFavorite;
    
    // Optimistic UI update
    m_isFavorite = newFavorite;
    emit isFavoriteChanged();

    std::weak_ptr<bool> alive = m_isAlive;
    QThreadPool::globalInstance()->start([this, alive, id, newFavorite]() {
        auto result = m_repository->getById(id);
        if (result.isSuccess() && result.value().has_value()) {
            auto item = result.value().value();
            item.favorite = newFavorite;
            m_repository->update(item);
        }
    });
}

void ViewerViewModel::next() {
    if (m_context) {
        m_context->next();
    }
}

void ViewerViewModel::previous() {
    if (m_context) {
        m_context->previous();
    }
}

void ViewerViewModel::zoom(qreal factor) {
    emit zoomRequested(factor);
}

void ViewerViewModel::pan(qreal dx, qreal dy) {
    emit panRequested(dx, dy);
}

void ViewerViewModel::fitToWindow() {
    emit fitToWindowRequested();
}

void ViewerViewModel::originalSize() {
    emit originalSizeRequested();
}

void ViewerViewModel::toggleFullscreen() {
    m_isFullscreen = !m_isFullscreen;
    emit isFullscreenChanged();
    emit fullscreenRequested(m_isFullscreen);
}

void ViewerViewModel::onContextMediaIdChanged() {
    if (!m_context) return;
    
    QString newMediaId = m_context->currentMediaId();
    if (newMediaId != m_mediaId) {
        m_mediaId = newMediaId;
        emit mediaIdChanged();
        
        m_rotation = 0; // Reset rotation on new image
        emit rotationChanged();

        loadMediaItem(m_mediaId);
    }
}

void ViewerViewModel::loadMediaItem(const QString& id) {
    int generation = ++m_loadGeneration;

    if (id.isEmpty()) {
        m_imageState = ImageState::Unavailable;
        emit imageStateChanged();
        
        m_canonicalPath.clear();
        m_title.clear();
        m_fileSize = 0;
        m_resolution = QSize();
        m_isFavorite = false;
        
        emit canonicalPathChanged();
        emit titleChanged();
        emit fileSizeChanged();
        emit resolutionChanged();
        emit isFavoriteChanged();
        return;
    }

    m_imageState = ImageState::Loading;
    emit imageStateChanged();

    std::weak_ptr<bool> alive = m_isAlive;
    std::string mediaId = id.toStdString();
    auto repo = m_repository;

    QThreadPool::globalInstance()->start([this, alive, repo, mediaId, generation]() {
        auto result = repo->getById(mediaId);
        std::optional<core::models::MediaItem> item;
        bool isSuccess = result.isSuccess();
        
        if (isSuccess) {
            item = result.value();
        }

        QMetaObject::invokeMethod(this, [this, alive, generation, isSuccess, item]() {
            if (!alive.lock() || generation != m_loadGeneration) {
                return;
            }
            onMediaItemLoaded(generation, isSuccess, item);
        });
    });
}

void ViewerViewModel::onMediaItemLoaded(int generation, bool isSuccess, std::optional<core::models::MediaItem> item) {
    if (generation != m_loadGeneration) {
        return; // Stale
    }

    if (!isSuccess) {
        m_imageState = ImageState::Error;
        emit imageStateChanged();
        return;
    }

    if (!item.has_value()) {
        m_imageState = ImageState::Unavailable;
        emit imageStateChanged();
        return;
    }

    const auto& mediaItem = item.value();

    m_canonicalPath = QString::fromStdString(mediaItem.canonicalPath);
    emit canonicalPathChanged();

    QFileInfo fileInfo(m_canonicalPath);
    m_title = fileInfo.fileName();
    emit titleChanged();

    m_fileSize = mediaItem.fileSize;
    emit fileSizeChanged();

    m_resolution = QSize(mediaItem.width.value_or(0), mediaItem.height.value_or(0));
    emit resolutionChanged();

    m_isFavorite = mediaItem.favorite;
    emit isFavoriteChanged();

    m_imageState = ImageState::Ready;
    emit imageStateChanged();
    
    // Attempt to load as animated media
    m_animatedController->loadMedia(m_canonicalPath);
}

void ViewerViewModel::onLibraryEvent(const core::events::LibraryEvent& event) {
    // If the event is about our current media item, update it
    if (event.type == core::events::LibraryEvent::Type::Updated) {
        if (QString::fromStdString(event.mediaId) == m_mediaId) {
            // Need to reload to get new details (e.g. favorite status)
            // But doing it async is fine, or we can just trigger a reload if needed
            std::weak_ptr<bool> alive = m_isAlive;
            std::string id = m_mediaId.toStdString();
            int gen = ++m_loadGeneration;
            auto repo = m_repository;
            
            QThreadPool::globalInstance()->start([this, alive, repo, id, gen]() {
                auto result = repo->getById(id);
                std::optional<core::models::MediaItem> item;
                if (result.isSuccess()) {
                    item = result.value();
                }
                QMetaObject::invokeMethod(this, [this, alive, gen, item]() {
                    if (!alive.lock() || gen != m_loadGeneration) return;
                    if (item.has_value()) {
                        m_isFavorite = item.value().favorite;
                        emit isFavoriteChanged();
                        // other metadata like resolution etc could also change theoretically
                    }
                });
            });
        }
    } else if (event.type == core::events::LibraryEvent::Type::Removed) {
        if (QString::fromStdString(event.mediaId) == m_mediaId) {
            // Handle the fact that our current media is gone
            // Just mark unavailable, MediaListContext should theoretically handle navigation 
            // if we were to emit something, but MediaListContext listens to events too 
            // and should update the totalCount and index. 
            // But we can set unavailable for safety.
            m_imageState = ImageState::Unavailable;
            emit imageStateChanged();
        }
    }
}

} // namespace mnemis::ui::controllers
