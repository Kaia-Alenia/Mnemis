#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include "ui/controllers/AnimatedMediaController.hpp"

namespace mnemis::ui::components {

class AnimatedFrameItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(mnemis::ui::controllers::AnimatedMediaController* controller READ controller WRITE setController NOTIFY controllerChanged)

public:
    explicit AnimatedFrameItem(QQuickItem* parent = nullptr);

    void paint(QPainter* painter) override;

    controllers::AnimatedMediaController* controller() const;
    void setController(controllers::AnimatedMediaController* controller);

signals:
    void controllerChanged();

private slots:
    void onFrameReady(const QImage& frame);

private:
    controllers::AnimatedMediaController* m_controller = nullptr;
    QImage m_currentFrame;
};

} // namespace mnemis::ui::components
