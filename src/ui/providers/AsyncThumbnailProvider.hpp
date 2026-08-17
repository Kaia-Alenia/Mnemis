#pragma once

#include <QQuickAsyncImageProvider>
#include <QQuickImageResponse>
#include <QImage>
#include <memory>

#include "core/thumbnails/IThumbnailEngine.hpp"

namespace mnemis::ui::providers {

class ThumbnailResponse : public QQuickImageResponse {
    Q_OBJECT
public:
    ThumbnailResponse(const QString &id, const QSize &requestedSize, core::thumbnails::IThumbnailEngine* engine);
    ~ThumbnailResponse() override;

    QQuickTextureFactory *textureFactory() const override;
    void cancel() override;

private slots:
    void handleThumbnailResult(const QImage& image);

private:
    core::thumbnails::IThumbnailEngine* m_engine;
    QImage m_image;
    core::thumbnails::ThumbnailTaskId m_taskId;
    bool m_cancelled = false;
    std::shared_ptr<bool> m_isAlive;
};

class AsyncThumbnailProvider : public QQuickAsyncImageProvider {
public:
    explicit AsyncThumbnailProvider(core::thumbnails::IThumbnailEngine* engine);
    
    QQuickImageResponse *requestImageResponse(const QString &id, const QSize &requestedSize) override;

private:
    core::thumbnails::IThumbnailEngine* m_engine;
};

} // namespace mnemis::ui::providers
