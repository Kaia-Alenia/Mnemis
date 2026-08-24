#pragma once
#include "plugins/api/IPlugin.hpp"
#include <QObject>
#include <QtPlugin>
#include <memory>
#include "KoalaSyncSession.hpp"
#include "transport/WebSocketTransport.hpp"
#include "protocol/ProtocolAdapter.hpp"

namespace mnemis::plugins::koalasync {

class KoalaSyncPlugin : public QObject, public IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IPlugin_iid)
    Q_INTERFACES(mnemis::plugins::IPlugin)

public:
    KoalaSyncPlugin();
    ~KoalaSyncPlugin() override = default;

    QString name() const override;
    void onStartup() override;
    void onMediaLoaded(const QString& mediaId) override;

private:
    std::unique_ptr<transport::WebSocketTransport> m_transport;
    std::unique_ptr<protocol::ProtocolAdapter> m_adapter;
    std::unique_ptr<KoalaSyncSession> m_session;
};

} // namespace mnemis::plugins::koalasync
