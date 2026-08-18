#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <mpv/client.h>
#include <mpv/render_gl.h>

#include "ui/controllers/PlaybackController.hpp"

namespace mnemis::ui::components {

class MpvVideoItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(mnemis::ui::controllers::PlaybackController* controller READ controller WRITE setController NOTIFY controllerChanged)

public:
    explicit MpvVideoItem(QQuickItem* parent = nullptr);
    ~MpvVideoItem() override;

    Renderer* createRenderer() const override;

    mnemis::ui::controllers::PlaybackController* controller() const;
    void setController(mnemis::ui::controllers::PlaybackController* controller);

signals:
    void controllerChanged();
    void renderRequested();

public slots:
    void onRenderRequested();

private:
    mnemis::ui::controllers::PlaybackController* m_controller = nullptr;
    mpv_render_context* m_mpv_gl = nullptr;
    mpv_handle* m_mpv = nullptr;

    void initMpvContext();
    void destroyMpvContext();
    
    // Renderer needs access to the render context
    friend class MpvRenderer;
};

} // namespace mnemis::ui::components
