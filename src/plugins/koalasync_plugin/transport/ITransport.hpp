#pragma once
#include <QString>
#include <QByteArray>
#include <functional>

namespace mnemis::plugins::koalasync::transport {

class ITransport {
public:
    virtual ~ITransport() = default;
    
    using MessageHandler = std::function<void(const QString&)>;
    using ConnectHandler = std::function<void()>;
    using DisconnectHandler = std::function<void(const QString&)>;

    virtual void connectToServer(const QString& url) = 0;
    virtual void disconnectFromServer() = 0;
    virtual void sendMessage(const QString& message) = 0;

    virtual void setMessageHandler(MessageHandler handler) = 0;
    virtual void setConnectHandler(ConnectHandler handler) = 0;
    virtual void setDisconnectHandler(DisconnectHandler handler) = 0;
};

} // namespace mnemis::plugins::koalasync::transport
