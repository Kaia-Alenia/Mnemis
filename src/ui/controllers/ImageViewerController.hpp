#include <QImage>
#pragma once

#include <QObject>
#include <QString>
#include <QVector>

#include "media/viewer/ViewerTypes.hpp"

namespace mnemis::ui {

class ImageViewerController final : public QObject {
    Q_OBJECT

    Q_PROPERTY(
        QString sourceUrl
        READ sourceUrl
        NOTIFY sourceChanged
    )

    Q_PROPERTY(
        QString sourcePath
        READ sourcePath
        NOTIFY sourceChanged
    )

    Q_PROPERTY(
        bool opened
        READ opened
        NOTIFY openedChanged
    )

    Q_PROPERTY(
        bool animated
        READ animated
        NOTIFY sourceChanged
    )

    Q_PROPERTY(
        bool spriteSheetMode
        READ spriteSheetMode
        WRITE setSpriteSheetMode
        NOTIFY spriteSheetModeChanged
    )

    Q_PROPERTY(
        qreal zoom
        READ zoom
        WRITE setZoom
        NOTIFY zoomChanged
    )

    Q_PROPERTY(
        bool gridEnabled
        READ gridEnabled
        WRITE setGridEnabled
        NOTIFY gridChanged
    )

    Q_PROPERTY(
        int gridCellWidth
        READ gridCellWidth
        WRITE setGridCellWidth
        NOTIFY gridChanged
    )

    Q_PROPERTY(
        int gridCellHeight
        READ gridCellHeight
        WRITE setGridCellHeight
        NOTIFY gridChanged
    )

    Q_PROPERTY(
        int frameWidth
        READ frameWidth
        WRITE setFrameWidth
        NOTIFY spriteChanged
    )

    Q_PROPERTY(
        int frameHeight
        READ frameHeight
        WRITE setFrameHeight
        NOTIFY spriteChanged
    )

    Q_PROPERTY(
        int frameColumns
        READ frameColumns
        WRITE setFrameColumns
        NOTIFY spriteChanged
    )

    Q_PROPERTY(
        int frameRows
        READ frameRows
        WRITE setFrameRows
        NOTIFY spriteChanged
    )

    Q_PROPERTY(
        int frameStart
        READ frameStart
        WRITE setFrameStart
        NOTIFY spriteChanged
    )

    Q_PROPERTY(
        int frameEnd
        READ frameEnd
        WRITE setFrameEnd
        NOTIFY spriteChanged
    )

    Q_PROPERTY(
        int frameIndex
        READ frameIndex
        WRITE setFrameIndex
        NOTIFY frameChanged
    )

    Q_PROPERTY(
        int fps
        READ fps
        WRITE setFps
        NOTIFY animationSettingsChanged
    )

    Q_PROPERTY(
        int animationSpeed
        READ animationSpeed
        WRITE setAnimationSpeed
        NOTIFY animationSettingsChanged
    )

    Q_PROPERTY(
        bool loop
        READ loop
        WRITE setLoop
        NOTIFY animationSettingsChanged
    )

    Q_PROPERTY(
        int pixelX
        READ pixelX
        NOTIFY pixelChanged
    )

    Q_PROPERTY(
        int pixelY
        READ pixelY
        NOTIFY pixelChanged
    )

    Q_PROPERTY(
        QString pixelHex
        READ pixelHex
        NOTIFY pixelChanged
    )

    Q_PROPERTY(
        QString pixelRGBA
        READ pixelRGBA
        NOTIFY pixelChanged
    )

    Q_PROPERTY(
        int imageWidth
        READ imageWidth
        NOTIFY imageInfoChanged
    )

    Q_PROPERTY(
        int imageHeight
        READ imageHeight
        NOTIFY imageInfoChanged
    )

    Q_PROPERTY(
        int totalFrames
        READ totalFrames
        WRITE setTotalFrames
        NOTIFY animationInfoChanged
    )

    Q_PROPERTY(
        int animationFrameCount
        READ animationFrameCount
        NOTIFY animationInfoChanged
    )

    Q_PROPERTY(
        int animationLoopCount
        READ animationLoopCount
        NOTIFY animationInfoChanged
    )

    Q_PROPERTY(
        int currentAnimationDelay
        READ currentAnimationDelay
        NOTIFY frameChanged
    )

public:
    explicit ImageViewerController(
        QObject* parent = nullptr
    );

    QString sourceUrl() const;
    QString sourcePath() const;

    bool opened() const;
    bool animated() const;

    bool spriteSheetMode() const;
    void setSpriteSheetMode(bool value);

    qreal zoom() const;
    void setZoom(qreal value);

    bool gridEnabled() const;
    void setGridEnabled(bool value);

    int gridCellWidth() const;
    void setGridCellWidth(int value);

    int gridCellHeight() const;
    void setGridCellHeight(int value);

    int frameWidth() const;
    void setFrameWidth(int value);

    int frameHeight() const;
    void setFrameHeight(int value);

    int frameColumns() const;
    void setFrameColumns(int value);

    int frameRows() const;
    void setFrameRows(int value);

    int frameStart() const;
    void setFrameStart(int value);

    int frameEnd() const;
    void setFrameEnd(int value);

    int frameIndex() const;
    void setFrameIndex(int value);

    int fps() const;
    void setFps(int value);

    int animationSpeed() const;
    void setAnimationSpeed(int value);

    bool loop() const;
    void setLoop(bool value);

    int pixelX() const;
    int pixelY() const;
    QString pixelHex() const;
    QString pixelRGBA() const;

    int imageWidth() const;
    int imageHeight() const;

    int totalFrames() const;
    void setTotalFrames(int value);

    int animationFrameCount() const;
    int animationLoopCount() const;
    int currentAnimationDelay() const;

    Q_INVOKABLE void openMedia(
        const QString& path
    );

    Q_INVOKABLE void close();

    Q_INVOKABLE void setPixelInfo(
        int x,
        int y,
        const QString& hex
    );

    Q_INVOKABLE void resetView();

signals:
    void sourceChanged();
    void openedChanged();

    void spriteSheetModeChanged();
    void zoomChanged();
    void gridChanged();
    void spriteChanged();
    void frameChanged();
    void animationSettingsChanged();
    void pixelChanged();
    void imageInfoChanged();
    void animationInfoChanged();

private:
    QString m_sourceUrl;
    QString m_sourcePath;

    bool m_opened = false;
    bool m_animated = false;

    bool m_spriteSheetMode = false;

    qreal m_zoom = 1.0;

    bool m_gridEnabled = false;

    int m_gridCellWidth = 1;
    int m_gridCellHeight = 1;

    int m_frameWidth = 48;
    int m_frameHeight = 48;

    int m_frameColumns = 1;
    int m_frameRows = 1;

    int m_frameStart = 0;
    int m_frameEnd = 0;
    int m_frameIndex = 0;

    int m_fps = 12;
    int m_animationSpeed = 100;

    bool m_loop = true;

    int m_pixelX = -1;
    int m_pixelY = -1;

    QString m_pixelHex = QStringLiteral("#00000000");
    QString m_pixelRGBA = QStringLiteral("0, 0, 0, 0");

    QImage m_currentImage;

    int m_imageWidth = 0;
    int m_imageHeight = 0;

    int m_totalFrames = 1;

    QVector<int> m_animationFrameDelays;
    int m_animationLoopCount = -1;
};

} // namespace mnemis::ui
