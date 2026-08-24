#include "KoalaSyncPlugin.hpp"
#include <QDebug>

namespace mnemis::plugins::koalasync {

KoalaSyncPlugin::KoalaSyncPlugin()
    : m_transport(std::make_unique<transport::WebSocketTransport>()),
      m_adapter(std::make_unique<protocol::ProtocolAdapter>()),
      m_session(std::make_unique<KoalaSyncSession>(m_transport.get(), m_adapter.get()))
{
    connect(m_session.get(), &KoalaSyncSession::connected, this, []() {
        qInfo() << "[KoalaSyncPlugin] Connected to session.";
    });
    connect(m_session.get(), &KoalaSyncSession::disconnected, this, [](const QString& reason) {
        qInfo() << "[KoalaSyncPlugin] Disconnected from session:" << reason;
    });
    connect(m_session.get(), &KoalaSyncSession::remotePlay, this, []() {
        qInfo() << "[KoalaSyncPlugin] Remote play requested.";
    });
    connect(m_session.get(), &KoalaSyncSession::remotePause, this, []() {
        qInfo() << "[KoalaSyncPlugin] Remote pause requested.";
    });
    connect(m_session.get(), &KoalaSyncSession::remoteSeek, this, [](double position) {
        qInfo() << "[KoalaSyncPlugin] Remote seek requested:" << position;
    });
}

QString KoalaSyncPlugin::name() const {
    return "KoalaSync";
}

void KoalaSyncPlugin::onStartup() {
    qInfo() << "[KoalaSyncPlugin] Starting up...";
}

void KoalaSyncPlugin::onMediaLoaded(const QString& mediaId) {
    qInfo() << "[KoalaSyncPlugin] Syncing media load:" << mediaId;
}

} // namespace mnemis::plugins::koalasync
