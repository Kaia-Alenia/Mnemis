#pragma once

#include <QObject>
#include <QString>
#include <QSize>
#include <memory>
#include <atomic>

#include "core/models/MediaItem.hpp"
#include "core/repositories/IMediaRepository.hpp"
#include "core/events/ILibraryEventBus.hpp"
#include "ui/controllers/MediaListContext.hpp"
#include "ui/controllers/AnimatedMediaController.hpp"

namespace mnemis::ui::controllers {

class ViewerViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString mediaId READ mediaId NOTIFY mediaIdChanged)
    Q_PROPERTY(QString canonicalPath READ canonicalPath NOTIFY canonicalPathChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString currentType READ currentType NOTIFY currentTypeChanged)
    Q_PROPERTY(qint64 fileSize READ fileSize NOTIFY fileSizeChanged)
    Q_PROPERTY(QSize resolution READ resolution NOTIFY resolutionChanged)
    Q_PROPERTY(bool isFavorite READ isFavorite NOTIFY isFavoriteChanged)
    Q_PROPERTY(QString imageState READ imageState NOTIFY imageStateChanged)
    Q_PROPERTY(int rotation READ rotation NOTIFY rotationChanged)
    Q_PROPERTY(bool isFullscreen READ isFullscreen NOTIFY isFullscreenChanged)
    Q_PROPERTY(mnemis::ui::controllers::AnimatedMediaController* animatedController READ animatedController CONSTANT)

public:
    enum class ImageState {
        Loading,
        Ready,
        Error,
        Unavailable
    };
    Q_ENUM(ImageState)

    explicit ViewerViewModel(
        core::repositories::IMediaRepository* repository,
        QObject* parent = nullptr);

    ~ViewerViewModel() override;

    void setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus);
    void setContext(MediaListContext* context);

    QString mediaId() const;
    QString canonicalPath() const;
    QString title() const;
    QString currentType() const;
    qint64 fileSize() const;
    QSize resolution() const;
    bool isFavorite() const;
    QString imageState() const;
    int rotation() const;
    bool isFullscreen() const;
    AnimatedMediaController* animatedController() const;

public slots:
    void open(const QString& mediaId);
    void rotate(int degrees);
    void toggleFavorite();

    // Navigation through Context
    void next();
    void previous();

    // UI visual commands (stub/signals for QML to bind to)
    void zoom(qreal factor);
    void pan(qreal dx, qreal dy);
    void fitToWindow();
    void originalSize();
    void toggleFullscreen();

signals:
    void mediaIdChanged();
    void canonicalPathChanged();
    void titleChanged();
    void currentTypeChanged();
    void fileSizeChanged();
    void resolutionChanged();
    void isFavoriteChanged();
    void imageStateChanged();
    void rotationChanged();
    void isFullscreenChanged();

    // UI visual signals
    void zoomRequested(qreal factor);
    void panRequested(qreal dx, qreal dy);
    void fitToWindowRequested();
    void originalSizeRequested();
    void fullscreenRequested(bool isFullscreen);

private slots:
    void onContextMediaIdChanged();
    void onLibraryEvent(const core::events::LibraryEvent& event);
    void loadMediaItem(const QString& id);
    void onMediaItemLoaded(int generation, bool isSuccess, std::optional<core::models::MediaItem> item);

private:
    core::repositories::IMediaRepository* m_repository;
    MediaListContext* m_context = nullptr;
    std::shared_ptr<core::events::ILibraryEventBus> m_eventBus;
    core::events::ILibraryEventBus::SubscriptionToken m_eventSubToken = 0;

    QString m_mediaId;
    QString m_canonicalPath;
    QString m_title;
    QString m_currentType;
    qint64 m_fileSize = 0;
    QSize m_resolution;
    bool m_isFavorite = false;
    ImageState m_imageState = ImageState::Unavailable;
    int m_rotation = 0;
    bool m_isFullscreen = false;

    AnimatedMediaController* m_animatedController = nullptr;

    std::shared_ptr<bool> m_isAlive;
    std::atomic<int> m_loadGeneration{0};
};

} // namespace mnemis::ui::controllers
