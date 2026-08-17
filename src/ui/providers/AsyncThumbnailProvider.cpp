#include "AsyncThumbnailProvider.hpp"
#include <QUrlQuery>
#include <QUrl>
#include <QQuickTextureFactory>
#include <QMetaObject>

namespace mnemis::ui::providers {

ThumbnailResponse::ThumbnailResponse(const QString &id, const QSize &requestedSize, core::thumbnails::IThumbnailEngine* engine)
    : m_engine(engine)
    , m_taskId(0)
    , m_isAlive(std::make_shared<bool>(true))
{
    QUrl url("dummy://" + id);
    QString mediaId = url.path();
    if (mediaId.startsWith('/')) mediaId.remove(0, 1);
    if (mediaId.isEmpty()) mediaId = url.host();
    
    QUrlQuery query(url.query());
    
    core::thumbnails::ThumbnailSpec spec;
    spec.mediaId = mediaId.toStdString();
    spec.canonicalPath = query.queryItemValue("path").toStdString();
    spec.fileSize = query.queryItemValue("size").toULongLong();
    spec.modifiedTime = query.queryItemValue("time").toLongLong();
    
    spec.targetWidth = requestedSize.width() > 0 ? requestedSize.width() : 256;
    spec.targetHeight = requestedSize.height() > 0 ? requestedSize.height() : 256;
    spec.version = 0;
    
    auto alive = m_isAlive;
    m_taskId = m_engine->requestThumbnail(spec, core::thumbnails::ThumbnailPriority::Visible, 
        [this, alive](const core::thumbnails::ThumbnailResult& result) {
            if (!*alive) return;
            
            QImage image;
            if (result.status == core::thumbnails::ThumbnailStatus::Ready && result.image.has_value()) {
                const auto& buf = result.image.value();
                if (buf.isEncoded) {
                    image.loadFromData(buf.data.data(), buf.data.size());
                } else {
                    QImage::Format fmt = QImage::Format_RGBA8888;
                    if (buf.channels == 3) fmt = QImage::Format_RGB888;
                    else if (buf.channels == 1) fmt = QImage::Format_Grayscale8;
                    
                    // We must copy because buf.data will be destroyed after callback
                    image = QImage(buf.data.data(), buf.width, buf.height, fmt).copy();
                }
            }
            
            QMetaObject::invokeMethod(this, "handleThumbnailResult", 
                Qt::QueuedConnection, Q_ARG(QImage, image));
        });
}

ThumbnailResponse::~ThumbnailResponse() {
    *m_isAlive = false;
    cancel();
}

QQuickTextureFactory *ThumbnailResponse::textureFactory() const {
    return QQuickTextureFactory::textureFactoryForImage(m_image);
}

void ThumbnailResponse::cancel() {
    if (!m_cancelled) {
        m_cancelled = true;
        if (m_taskId != 0) {
            m_engine->cancelRequest(m_taskId);
            m_taskId = 0;
        }
    }
}

void ThumbnailResponse::handleThumbnailResult(const QImage& image) {
    if (m_cancelled) return;
    
    m_image = image;
    emit finished();
}

AsyncThumbnailProvider::AsyncThumbnailProvider(core::thumbnails::IThumbnailEngine* engine)
    : m_engine(engine)
{
}

QQuickImageResponse *AsyncThumbnailProvider::requestImageResponse(const QString &id, const QSize &requestedSize) {
    return new ThumbnailResponse(id, requestedSize, m_engine);
}

} // namespace mnemis::ui::providers
