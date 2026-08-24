#pragma once
#include <QObject>
#include <QString>
#include "transport/ITransport.hpp"
#include "protocol/IProtocolAdapter.hpp"

namespace mnemis::plugins::koalasync {

class KoalaSyncSession : public QObject {
    Q_OBJECT
public:
    explicit KoalaSyncSession(
        transport::ITransport* transport,
        protocol::IProtocolAdapter* adapter,
        QObject* parent = nullptr);

    void connectToServer(const QString& url, const QString& roomId);
    void disconnect();

    void sendPlay();
    void sendPause();
    void sendSeek(double position);

signals:
    void connected();
    void disconnected(const QString& reason);
    void remotePlay();
    void remotePause();
    void remoteSeek(double position);

private:
    void handleEvent(const QString& eventName, const QJsonObject& payload);

    transport::ITransport* m_transport;
    protocol::IProtocolAdapter* m_adapter;
    QString m_roomId;
};

} // namespace mnemis::plugins::koalasync
