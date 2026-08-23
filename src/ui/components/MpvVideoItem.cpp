#include "MpvVideoItem.hpp"
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QWindow>
#include <QQuickWindow>
#include <stdexcept>
#include <iostream>

namespace mnemis::ui::components {

static void on_mpv_render_update(void *ctx) {
    auto *item = static_cast<MpvVideoItem*>(ctx);
    emit item->renderRequested();
}

static void* get_proc_address(void* ctx, const char* name) {
    Q_UNUSED(ctx);
    QOpenGLContext *glctx = QOpenGLContext::currentContext();
    if (!glctx) return nullptr;
    return reinterpret_cast<void*>(glctx->getProcAddress(QByteArray(name)));
}

class MpvRenderer : public QQuickFramebufferObject::Renderer {
public:
    MpvRenderer(MpvVideoItem* item) : m_item(item) { }

    void render() override {
        if (!m_item->m_mpv) {
            return;
        }

        if (!m_item->m_mpv_gl) {
            m_item->window()->beginExternalCommands();
            
            mpv_opengl_init_params gl_init_params;
            gl_init_params.get_proc_address = get_proc_address;
            gl_init_params.get_proc_address_ctx = nullptr;

            mpv_render_param params[] = {
                {MPV_RENDER_PARAM_API_TYPE, const_cast<char*>(MPV_RENDER_API_TYPE_OPENGL)},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_init_params},
                {MPV_RENDER_PARAM_INVALID, nullptr}
            };

            int err = mpv_render_context_create(&m_item->m_mpv_gl, m_item->m_mpv, params);
            if (err < 0) {
                std::cerr << "Failed to initialize mpv GL context: " << mpv_error_string(err) << std::endl;
                m_item->window()->endExternalCommands();
                return;
            }
            mpv_render_context_set_update_callback(m_item->m_mpv_gl, on_mpv_render_update, m_item);

            m_item->window()->endExternalCommands();
            return;
        }

        m_item->window()->beginExternalCommands();

        // Render mpv output
        mpv_opengl_fbo mpfbo;
        mpfbo.fbo = framebufferObject()->handle();
        mpfbo.w = framebufferObject()->width();
        mpfbo.h = framebufferObject()->height();
        mpfbo.internal_format = 0; // mpv will figure it out

        mpv_render_param params[] = {
            {MPV_RENDER_PARAM_OPENGL_FBO, &mpfbo},
            {MPV_RENDER_PARAM_INVALID, nullptr}
        };

        mpv_render_context_render(m_item->m_mpv_gl, params);
        
        m_item->window()->endExternalCommands();
    }

    QOpenGLFramebufferObject* createFramebufferObject(const QSize &size) override {
        return new QOpenGLFramebufferObject(size);
    }

private:
    MpvVideoItem* m_item;
};

MpvVideoItem::MpvVideoItem(QQuickItem* parent) 
    : QQuickFramebufferObject(parent) {
    connect(this, &MpvVideoItem::renderRequested, this, &MpvVideoItem::onRenderRequested, Qt::QueuedConnection);
}

MpvVideoItem::~MpvVideoItem() {
    destroyMpvContext();
}

QQuickFramebufferObject::Renderer* MpvVideoItem::createRenderer() const {
    return new MpvRenderer(const_cast<MpvVideoItem*>(this));
}

mnemis::ui::controllers::PlaybackController* MpvVideoItem::controller() const {
    return m_controller;
}

void MpvVideoItem::setController(mnemis::ui::controllers::PlaybackController* controller) {
    if (m_controller != controller) {
        destroyMpvContext();
        m_controller = controller;
        initMpvContext();
        emit controllerChanged();
    }
}

void MpvVideoItem::onRenderRequested() {
    update();
}

void MpvVideoItem::initMpvContext() {
    if (!m_controller) return;

    m_mpv = static_cast<mpv_handle*>(m_controller->getNativePlayer());
    if (!m_mpv) return;

    // Trigger an update to force the renderer to be created and initialize the GL context on the render thread.
    update();
}

void MpvVideoItem::destroyMpvContext() {
    if (m_mpv_gl) {
        mpv_render_context_set_update_callback(m_mpv_gl, nullptr, nullptr);
        mpv_render_context_free(m_mpv_gl);
        m_mpv_gl = nullptr;
    }
    m_mpv = nullptr;
}

} // namespace mnemis::ui::components
