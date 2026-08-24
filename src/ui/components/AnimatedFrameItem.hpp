#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include "ui/controllers/AnimatedMediaController.hpp"

namespace mnemis::ui::components {

class AnimatedFrameItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(mnemis::ui::controllers::AnimatedMediaController* controller READ controller WRITE setController NOTIFY controllerChanged)
    Q_PROPERTY(bool smooth READ smooth WRITE setSmooth NOTIFY smoothChanged)

public:
    explicit AnimatedFrameItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    controllers::AnimatedMediaController* controller() const;
    void setController(controllers::AnimatedMediaController* controller);

    bool smooth() const { return m_smooth; }
    void setSmooth(bool s) { if (m_smooth != s) { m_smooth = s; emit smoothChanged(); update(); } }

signals:
    void controllerChanged();
    void smoothChanged();

private slots:
    void onFrameReady(const QImage& frame);

private:
    controllers::AnimatedMediaController* m_controller = nullptr;
    QImage m_currentFrame;
    bool m_smooth = false;  // false = nearest-neighbor (pixel-perfect for pixel art)
};

} // namespace mnemis::ui::components
