#include "QtConfig.hpp"
#include <QStringList>

namespace mnemis::infrastructure::config {

QtConfig::QtConfig() : m_settings() {
}

std::string QtConfig::getValue(std::string_view key, std::string_view defaultValue) const {
    QString qKey = QString::fromUtf8(key.data(), key.size());
    QString qDefault = QString::fromUtf8(defaultValue.data(), defaultValue.size());
    return m_settings.value(qKey, qDefault).toString().toStdString();
}

void QtConfig::setValue(std::string_view key, std::string_view value) {
    QString qKey = QString::fromUtf8(key.data(), key.size());
    QString qVal = QString::fromUtf8(value.data(), value.size());
    m_settings.setValue(qKey, qVal);
}

std::vector<std::string> QtConfig::getStringList(std::string_view key) const {
    QString qKey = QString::fromUtf8(key.data(), key.size());
    QStringList list = m_settings.value(qKey).toStringList();
    std::vector<std::string> result;
    result.reserve(list.size());
    for (const auto& str : list) {
        result.push_back(str.toStdString());
    }
    return result;
}

void QtConfig::setStringList(std::string_view key, const std::vector<std::string>& list) {
    QString qKey = QString::fromUtf8(key.data(), key.size());
    QStringList qList;
    qList.reserve(list.size());
    for (const auto& str : list) {
        qList.push_back(QString::fromStdString(str));
    }
    m_settings.setValue(qKey, qList);
}

bool QtConfig::getBool(std::string_view key, bool defaultValue) const {
    QString qKey = QString::fromUtf8(key.data(), key.size());
    return m_settings.value(qKey, defaultValue).toBool();
}

void QtConfig::setBool(std::string_view key, bool value) {
    QString qKey = QString::fromUtf8(key.data(), key.size());
    m_settings.setValue(qKey, value);
}

} // namespace mnemis::infrastructure::config
