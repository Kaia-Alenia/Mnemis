#include "ImageViewerController.hpp"

#include <QFileInfo>
#include <QImageReader>
#include <QUrl>

namespace mnemis::ui {

ImageViewerController::ImageViewerController(
    QObject* parent
)
    : QObject(parent)
{
}

QString ImageViewerController::sourceUrl() const
{
    return m_sourceUrl;
}

QString ImageViewerController::sourcePath() const
{
    return m_sourcePath;
}

bool ImageViewerController::opened() const
{
    return m_opened;
}

bool ImageViewerController::animated() const
{
    return m_animated;
}

bool ImageViewerController::spriteSheetMode() const
{
    return m_spriteSheetMode;
}

void ImageViewerController::setSpriteSheetMode(
    bool value
)
{
    if (m_spriteSheetMode == value) {
        return;
    }

    m_spriteSheetMode = value;

    emit spriteSheetModeChanged();

    resetView();
}

qreal ImageViewerController::zoom() const
{
    return m_zoom;
}

void ImageViewerController::setZoom(
    qreal value
)
{
    const qreal clamped =
        qBound(
            0.05,
            value,
            64.0
        );

    if (qFuzzyCompare(
            m_zoom,
            clamped
        )) {
        return;
    }

    m_zoom = clamped;

    emit zoomChanged();
}

bool ImageViewerController::gridEnabled() const
{
    return m_gridEnabled;
}

void ImageViewerController::setGridEnabled(
    bool value
)
{
    if (m_gridEnabled == value) {
        return;
    }

    m_gridEnabled = value;

    emit gridChanged();
}

int ImageViewerController::gridCellWidth() const
{
    return m_gridCellWidth;
}

void ImageViewerController::setGridCellWidth(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            4096
        );

    if (m_gridCellWidth == clamped) {
        return;
    }

    m_gridCellWidth = clamped;

    emit gridChanged();
}

int ImageViewerController::gridCellHeight() const
{
    return m_gridCellHeight;
}

void ImageViewerController::setGridCellHeight(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            4096
        );

    if (m_gridCellHeight == clamped) {
        return;
    }

    m_gridCellHeight = clamped;

    emit gridChanged();
}

int ImageViewerController::frameWidth() const
{
    return m_frameWidth;
}

void ImageViewerController::setFrameWidth(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            4096
        );

    if (m_frameWidth == clamped) {
        return;
    }

    m_frameWidth = clamped;

    emit spriteChanged();
}

int ImageViewerController::frameHeight() const
{
    return m_frameHeight;
}

void ImageViewerController::setFrameHeight(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            4096
        );

    if (m_frameHeight == clamped) {
        return;
    }

    m_frameHeight = clamped;

    emit spriteChanged();
}

int ImageViewerController::frameColumns() const
{
    return m_frameColumns;
}

void ImageViewerController::setFrameColumns(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            4096
        );

    if (m_frameColumns == clamped) {
        return;
    }

    m_frameColumns = clamped;

    emit spriteChanged();
}

int ImageViewerController::frameRows() const
{
    return m_frameRows;
}

void ImageViewerController::setFrameRows(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            4096
        );

    if (m_frameRows == clamped) {
        return;
    }

    m_frameRows = clamped;

    emit spriteChanged();
}

int ImageViewerController::frameStart() const
{
    return m_frameStart;
}

void ImageViewerController::setFrameStart(
    int value
)
{
    const int clamped =
        qMax(
            0,
            value
        );

    if (m_frameStart == clamped) {
        return;
    }

    m_frameStart = clamped;

    if (m_frameEnd < m_frameStart) {
        m_frameEnd = m_frameStart;
    }

    m_frameIndex =
        qBound(
            m_frameStart,
            m_frameIndex,
            m_frameEnd
        );

    emit spriteChanged();
    emit frameChanged();
}

int ImageViewerController::frameEnd() const
{
    return m_frameEnd;
}

void ImageViewerController::setFrameEnd(
    int value
)
{
    const int clamped =
        qMax(
            m_frameStart,
            value
        );

    if (m_frameEnd == clamped) {
        return;
    }

    m_frameEnd = clamped;

    m_frameIndex =
        qBound(
            m_frameStart,
            m_frameIndex,
            m_frameEnd
        );

    emit spriteChanged();
    emit frameChanged();
}

int ImageViewerController::frameIndex() const
{
    return m_frameIndex;
}

void ImageViewerController::setFrameIndex(
    int value
)
{
    const int clamped =
        qBound(
            m_frameStart,
            value,
            qMax(
                m_frameStart,
                m_frameEnd
            )
        );

    if (m_frameIndex == clamped) {
        return;
    }

    m_frameIndex = clamped;

    emit frameChanged();
}

int ImageViewerController::fps() const
{
    return m_fps;
}

void ImageViewerController::setFps(
    int value
)
{
    const int clamped =
        qBound(
            1,
            value,
            240
        );

    if (m_fps == clamped) {
        return;
    }

    m_fps = clamped;

    emit animationSettingsChanged();
}

int ImageViewerController::animationSpeed() const
{
    return m_animationSpeed;
}

void ImageViewerController::setAnimationSpeed(
    int value
)
{
    const int clamped =
        qBound(
            10,
            value,
            400
        );

    if (m_animationSpeed == clamped) {
        return;
    }

    m_animationSpeed = clamped;

    emit animationSettingsChanged();
}

bool ImageViewerController::loop() const
{
    return m_loop;
}

void ImageViewerController::setLoop(
    bool value
)
{
    if (m_loop == value) {
        return;
    }

    m_loop = value;

    emit animationSettingsChanged();
}

int ImageViewerController::pixelX() const
{
    return m_pixelX;
}

int ImageViewerController::pixelY() const
{
    return m_pixelY;
}

QString ImageViewerController::pixelHex() const
{
    return m_pixelHex;
}

int ImageViewerController::imageWidth() const
{
    return m_imageWidth;
}

int ImageViewerController::imageHeight() const
{
    return m_imageHeight;
}

int ImageViewerController::totalFrames() const
{
    return m_totalFrames;
}

void ImageViewerController::setTotalFrames(
    int value
)
{
    const int clamped =
        qMax(
            1,
            value
        );

    if (m_totalFrames == clamped) {
        return;
    }

    m_totalFrames = clamped;

    m_frameEnd =
        qMax(
            m_frameEnd,
            m_totalFrames - 1
        );

    emit animationInfoChanged();
    emit spriteChanged();
}

void ImageViewerController::openMedia(
    const QString& path
)
{
    QFileInfo information(path);

    if (!information.exists() ||
        !information.isFile()) {

        close();

        return;
    }

    m_sourcePath =
        information.absoluteFilePath();

    m_sourceUrl =
        QUrl::fromLocalFile(
            m_sourcePath
        ).toString();

    const QString suffix =
        information.suffix()
            .toLower();

    m_animated =
        suffix == QStringLiteral("gif") ||
        suffix == QStringLiteral("apng") ||
        suffix == QStringLiteral("webp");

    QImageReader reader(
        m_sourcePath
    );

    QSize size =
        reader.size();

    if (size.isValid()) {
        m_imageWidth =
            size.width();

        m_imageHeight =
            size.height();
    } else {
        m_imageWidth = 0;
        m_imageHeight = 0;
    }

    m_totalFrames = 1;

    if (m_animated) {
        const int count =
            reader.imageCount();

        if (count > 0) {
            m_totalFrames = count;
        }
    }

    m_frameStart = 0;

    m_frameEnd =
        qMax(
            0,
            m_totalFrames - 1
        );

    m_frameIndex = 0;

    m_opened = true;

    resetView();

    emit sourceChanged();
    emit openedChanged();
    emit imageInfoChanged();
    emit animationInfoChanged();
    emit frameChanged();
}

void ImageViewerController::close()
{
    const bool wasOpened =
        m_opened;

    m_opened = false;

    m_sourcePath.clear();
    m_sourceUrl.clear();

    m_animated = false;

    m_imageWidth = 0;
    m_imageHeight = 0;

    m_totalFrames = 1;

    m_frameIndex = 0;
    m_frameStart = 0;
    m_frameEnd = 0;

    m_pixelX = -1;
    m_pixelY = -1;
    m_pixelHex =
        QStringLiteral(
            "#00000000"
        );

    if (wasOpened) {
        emit openedChanged();
    }

    emit sourceChanged();
    emit imageInfoChanged();
    emit animationInfoChanged();
    emit frameChanged();
    emit pixelChanged();
}

void ImageViewerController::setPixelInfo(
    int x,
    int y,
    const QString& hex
)
{
    if (m_pixelX == x &&
        m_pixelY == y &&
        m_pixelHex == hex) {
        return;
    }

    m_pixelX = x;
    m_pixelY = y;
    m_pixelHex = hex;

    emit pixelChanged();
}

void ImageViewerController::resetView()
{
    m_zoom = 1.0;

    m_gridEnabled = false;

    m_gridCellWidth = 1;
    m_gridCellHeight = 1;

    m_pixelX = -1;
    m_pixelY = -1;

    m_pixelHex =
        QStringLiteral(
            "#00000000"
        );

    emit zoomChanged();
    emit gridChanged();
    emit pixelChanged();
}

} // namespace mnemis::ui
