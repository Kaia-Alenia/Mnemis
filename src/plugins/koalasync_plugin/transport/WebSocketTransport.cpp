#include "WebSocketTransport.hpp"
#include <QDebug>
#include <QUrl>

namespace mnemis::plugins::koalasync::transport {

WebSocketTransport::WebSocketTransport(QObject* parent)
    : QObject(parent), m_socket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    connect(m_socket, &QWebSocket::connected, this, &WebSocketTransport::onConnected);
    connect(m_socket, &QWebSocket::disconnected, this, &WebSocketTransport::onDisconnected);
    connect(m_socket, &QWebSocket::textMessageReceived, this, &WebSocketTransport::onTextMessageReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WebSocketTransport::onErrorOccurred);
}

WebSocketTransport::~WebSocketTransport() {
    disconnectFromServer();
}

void WebSocketTransport::connectToServer(const QString& url) {
    qInfo() << "[WebSocketTransport] Connecting to" << url;
    m_socket->open(QUrl(url));
}

void WebSocketTransport::disconnectFromServer() {
    if (m_socket->isValid()) {
        m_socket->close();
    }
}

void WebSocketTransport::sendMessage(const QString& message) {
    if (m_socket->isValid()) {
        m_socket->sendTextMessage(message);
    } else {
        qWarning() << "[WebSocketTransport] Cannot send message, socket not connected";
    }
}

void WebSocketTransport::setMessageHandler(MessageHandler handler) {
    m_messageHandler = std::move(handler);
}

void WebSocketTransport::setConnectHandler(ConnectHandler handler) {
    m_connectHandler = std::move(handler);
}

void WebSocketTransport::setDisconnectHandler(DisconnectHandler handler) {
    m_disconnectHandler = std::move(handler);
}

void WebSocketTransport::onConnected() {
    qInfo() << "[WebSocketTransport] Connected!";
    if (m_connectHandler) {
        m_connectHandler();
    }
}

void WebSocketTransport::onDisconnected() {
    qInfo() << "[WebSocketTransport] Disconnected!";
    if (m_disconnectHandler) {
        m_disconnectHandler(m_socket->errorString());
    }
}

void WebSocketTransport::onTextMessageReceived(const QString& message) {
    if (m_messageHandler) {
        m_messageHandler(message);
    }
}

void WebSocketTransport::onErrorOccurred(QAbstractSocket::SocketError error) {
    qWarning() << "[WebSocketTransport] Error occurred:" << error << m_socket->errorString();
}

} // namespace mnemis::plugins::koalasync::transport
