#pragma once
#include "IProtocolAdapter.hpp"
#include <QObject>

namespace mnemis::plugins::koalasync::protocol {

class ProtocolAdapter : public QObject, public IProtocolAdapter {
    Q_OBJECT
public:
    explicit ProtocolAdapter(QObject* parent = nullptr);
    ~ProtocolAdapter() override = default;

    void parseIncomingMessage(const QString& rawMessage) override;
    QString formatOutgoingMessage(const QString& eventName, const QJsonObject& payload) override;
    void setEventCallback(EventCallback callback) override;

private:
    EventCallback m_callback;
};

} // namespace mnemis::plugins::koalasync::protocol
