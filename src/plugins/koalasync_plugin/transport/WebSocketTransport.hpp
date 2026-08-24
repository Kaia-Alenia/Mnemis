#pragma once
#include "ITransport.hpp"
#include <QObject>
#include <QWebSocket>

namespace mnemis::plugins::koalasync::transport {

class WebSocketTransport : public QObject, public ITransport {
    Q_OBJECT
public:
    explicit WebSocketTransport(QObject* parent = nullptr);
    ~WebSocketTransport() override;

    void connectToServer(const QString& url) override;
    void disconnectFromServer() override;
    void sendMessage(const QString& message) override;

    void setMessageHandler(MessageHandler handler) override;
    void setConnectHandler(ConnectHandler handler) override;
    void setDisconnectHandler(DisconnectHandler handler) override;

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QWebSocket* m_socket;
    MessageHandler m_messageHandler;
    ConnectHandler m_connectHandler;
    DisconnectHandler m_disconnectHandler;
};

} // namespace mnemis::plugins::koalasync::transport
