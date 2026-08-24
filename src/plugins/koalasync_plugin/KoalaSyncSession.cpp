#include "KoalaSyncSession.hpp"
#include <QDebug>
#include <QJsonObject>

namespace mnemis::plugins::koalasync {

KoalaSyncSession::KoalaSyncSession(
    transport::ITransport* transport,
    protocol::IProtocolAdapter* adapter,
    QObject* parent)
    : QObject(parent), m_transport(transport), m_adapter(adapter)
{
    m_transport->setConnectHandler([this]() {
        QJsonObject payload;
        payload["roomId"] = m_roomId;
        QString msg = m_adapter->formatOutgoingMessage("join_room", payload);
        m_transport->sendMessage(msg);
        emit connected();
    });

    m_transport->setDisconnectHandler([this](const QString& reason) {
        emit disconnected(reason);
    });

    m_transport->setMessageHandler([this](const QString& msg) {
        m_adapter->parseIncomingMessage(msg);
    });

    m_adapter->setEventCallback([this](const QString& eventName, const QJsonObject& payload) {
        handleEvent(eventName, payload);
    });
}

void KoalaSyncSession::connectToServer(const QString& url, const QString& roomId) {
    m_roomId = roomId;
    m_transport->connectToServer(url);
}

void KoalaSyncSession::disconnect() {
    m_transport->disconnectFromServer();
}

void KoalaSyncSession::sendPlay() {
    QString msg = m_adapter->formatOutgoingMessage("play", QJsonObject());
    m_transport->sendMessage(msg);
}

void KoalaSyncSession::sendPause() {
    QString msg = m_adapter->formatOutgoingMessage("pause", QJsonObject());
    m_transport->sendMessage(msg);
}

void KoalaSyncSession::sendSeek(double position) {
    QJsonObject payload;
    payload["time"] = position;
    QString msg = m_adapter->formatOutgoingMessage("seek", payload);
    m_transport->sendMessage(msg);
}

void KoalaSyncSession::handleEvent(const QString& eventName, const QJsonObject& payload) {
    if (eventName == "play") {
        emit remotePlay();
    } else if (eventName == "pause") {
        emit remotePause();
    } else if (eventName == "seek") {
        if (payload.contains("time") && payload["time"].isDouble()) {
            emit remoteSeek(payload["time"].toDouble());
        }
    }
}

} // namespace mnemis::plugins::koalasync
