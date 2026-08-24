#include "ProtocolAdapter.hpp"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>

namespace mnemis::plugins::koalasync::protocol {

ProtocolAdapter::ProtocolAdapter(QObject* parent) : QObject(parent) {}

void ProtocolAdapter::setEventCallback(EventCallback callback) {
    m_callback = std::move(callback);
}

void ProtocolAdapter::parseIncomingMessage(const QString& rawMessage) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(rawMessage.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError) {
        qWarning() << "[ProtocolAdapter] Failed to parse message:" << error.errorString();
        return;
    }

    if (!doc.isObject()) {
        qWarning() << "[ProtocolAdapter] Message is not a JSON object";
        return;
    }

    QJsonObject obj = doc.object();
    if (!obj.contains("event") || !obj["event"].isString()) {
        qWarning() << "[ProtocolAdapter] Message missing 'event' string field";
        return;
    }

    QString eventName = obj["event"].toString();
    QJsonObject payload;
    if (obj.contains("data") && obj["data"].isObject()) {
        payload = obj["data"].toObject();
    }

    if (m_callback) {
        m_callback(eventName, payload);
    }
}

QString ProtocolAdapter::formatOutgoingMessage(const QString& eventName, const QJsonObject& payload) {
    QJsonObject obj;
    obj["event"] = eventName;
    if (!payload.isEmpty()) {
        obj["data"] = payload;
    }

    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

} // namespace mnemis::plugins::koalasync::protocol
