#include "AnimatedFrameItem.hpp"
#include <QPainter>

namespace mnemis::ui::components {

AnimatedFrameItem::AnimatedFrameItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
}

void AnimatedFrameItem::paint(QPainter* painter) {
    if (!m_currentFrame.isNull()) {
        // Use FastTransformation (nearest-neighbor) for pixel-perfect rendering.
        // SmoothTransformation blurs pixel art — always use Fast for animated sprites.
        Qt::TransformationMode mode = m_smooth ? Qt::SmoothTransformation : Qt::FastTransformation;
        QImage scaledImage = m_currentFrame.scaled(
            boundingRect().size().toSize(),
            Qt::KeepAspectRatio,
            mode
        );

        // Center the image
        int x = (boundingRect().width() - scaledImage.width()) / 2;
        int y = (boundingRect().height() - scaledImage.height()) / 2;

        painter->drawImage(x, y, scaledImage);
    }
}

controllers::AnimatedMediaController* AnimatedFrameItem::controller() const {
    return m_controller;
}

void AnimatedFrameItem::setController(controllers::AnimatedMediaController* controller) {
    if (m_controller == controller) {
        return;
    }

    if (m_controller) {
        disconnect(m_controller, &controllers::AnimatedMediaController::frameReady,
                   this, &AnimatedFrameItem::onFrameReady);
    }

    m_controller = controller;

    if (m_controller) {
        connect(m_controller, &controllers::AnimatedMediaController::frameReady,
                this, &AnimatedFrameItem::onFrameReady);
    }

    emit controllerChanged();
    
    // Clear frame if controller is null
    if (!m_controller) {
        m_currentFrame = QImage();
        update();
    }
}

void AnimatedFrameItem::onFrameReady(const QImage& frame) {
    m_currentFrame = frame;
    update(); // Request a repaint
}

} // namespace mnemis::ui::components
