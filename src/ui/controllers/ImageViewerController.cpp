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

    if (m_spriteSheetMode) {
        m_frameEnd =
            qMax(
                0,
                m_frameColumns * m_frameRows - 1
            );

        m_frameIndex =
            qBound(
                m_frameStart,
                m_frameIndex,
                m_frameEnd
            );

        emit frameChanged();
    }

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

    if (m_spriteSheetMode) {
        m_frameEnd =
            qMax(
                0,
                m_frameColumns * m_frameRows - 1
            );

        m_frameIndex =
            qBound(
                m_frameStart,
                m_frameIndex,
                m_frameEnd
            );

        emit frameChanged();
    }

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

QString ImageViewerController::pixelRGBA() const
{
    return m_pixelRGBA;
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

int ImageViewerController::animationFrameCount() const
{
    return m_animationFrameDelays.isEmpty()
        ? 1
        : m_animationFrameDelays.size();
}

int ImageViewerController::animationLoopCount() const
{
    return m_animationLoopCount;
}

int ImageViewerController::currentAnimationDelay() const
{
    if (
        m_animationFrameDelays.isEmpty() ||
        m_frameIndex < 0 ||
        m_frameIndex >= m_animationFrameDelays.size()
    ) {
        return 0;
    }

    return m_animationFrameDelays.at(
        m_frameIndex
    );
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

    m_currentImage =
        QImage(
            m_sourcePath
        );

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

    // --------------------------------------------------------
    // Generic animation detection
    //
    // Do not rely on extension or imageCount().
    // Walk the real decoded image sequence.
    // --------------------------------------------------------

    m_animationFrameDelays.clear();
    m_animationLoopCount = -1;

    m_totalFrames = 1;
    m_animated = false;

    QImageReader animationReader(
        m_sourcePath
    );

    if (animationReader.canRead()) {

        while (true) {

            const QImage frame =
                animationReader.read();

            if (frame.isNull()) {
                break;
            }

            int delay =
                animationReader.nextImageDelay();

            if (delay <= 0) {
                delay = 83;
            }

            m_animationFrameDelays.append(
                delay
            );

            if (!animationReader.jumpToNextImage()) {
                break;
            }
        }

        if (
            m_animationFrameDelays.size() > 1
        ) {
            m_animated = true;

            m_totalFrames =
                m_animationFrameDelays.size();

            QImageReader loopReader(
                m_sourcePath
            );

            m_animationLoopCount =
                loopReader.loopCount();
        } else {
            m_animationFrameDelays.clear();
            m_totalFrames = 1;
            m_animationLoopCount = -1;
        }
    }
    m_frameStart = 0;

    m_frameEnd =
        qMax(
            0,
            m_totalFrames - 1
        );

    m_frameIndex = 0;

    if (m_animated) {
        m_frameStart = 0;
        m_frameEnd =
            qMax(
                0,
                m_totalFrames - 1
            );
    }

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

    m_currentImage =
        QImage();

    m_animated = false;

    m_imageWidth = 0;
    m_imageHeight = 0;

    m_totalFrames = 1;
    m_animationFrameDelays.clear();
    m_animationLoopCount = -1;

    m_frameIndex = 0;
    m_frameStart = 0;
    m_frameEnd = 0;

    m_pixelX = -1;
    m_pixelY = -1;
    m_pixelHex =
        QStringLiteral(
            "#00000000"
        );

    m_pixelRGBA =
        QStringLiteral(
            "0, 0, 0, 0"
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
    const QString&)
{
    if (m_currentImage.isNull() || x < 0 || y < 0) {
        return;
    }

    int sourceX = x;
    int sourceY = y;

    if (m_spriteSheetMode) {
        const int column =
            m_frameColumns > 0 ? (m_frameIndex % m_frameColumns) : 0;
        const int row =
            m_frameColumns > 0 ? (m_frameIndex / m_frameColumns) : 0;

        sourceX = column * m_frameWidth + x;
        sourceY = row * m_frameHeight + y;
    }

    if (!m_currentImage.valid(sourceX, sourceY)) {
        return;
    }

    const QColor color =
        m_currentImage.pixelColor(sourceX, sourceY);

    const QString hex =
        QStringLiteral("#%1%2%3%4")
            .arg(color.red(), 2, 16, QChar('0'))
            .arg(color.green(), 2, 16, QChar('0'))
            .arg(color.blue(), 2, 16, QChar('0'))
            .arg(color.alpha(), 2, 16, QChar('0'))
            .toUpper();

    const QString rgba =
        QStringLiteral("%1, %2, %3, %4")
            .arg(color.red())
            .arg(color.green())
            .arg(color.blue())
            .arg(color.alpha());

    m_pixelX = x;
    m_pixelY = y;
    m_pixelHex = hex;
    m_pixelRGBA = rgba;

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

    m_pixelRGBA =
        QStringLiteral(
            "0, 0, 0, 0"
        );

    emit zoomChanged();
    emit gridChanged();
    emit pixelChanged();
}

} // namespace mnemis::ui
