#pragma once
#include <QString>
#include <QJsonObject>
#include <functional>

namespace mnemis::plugins::koalasync::protocol {

class IProtocolAdapter {
public:
    virtual ~IProtocolAdapter() = default;

    using EventCallback = std::function<void(const QString& eventName, const QJsonObject& payload)>;

    virtual void parseIncomingMessage(const QString& rawMessage) = 0;
    virtual QString formatOutgoingMessage(const QString& eventName, const QJsonObject& payload) = 0;
    virtual void setEventCallback(EventCallback callback) = 0;
};

} // namespace mnemis::plugins::koalasync::protocol
