#pragma once

#include "core/IConfig.hpp"
#include <QSettings>

namespace mnemis::infrastructure::config {

class QtConfig : public core::IConfig {
public:
    QtConfig();
    ~QtConfig() override = default;

    std::string getValue(std::string_view key, std::string_view defaultValue = "") const override;
    void setValue(std::string_view key, std::string_view value) override;

    std::vector<std::string> getStringList(std::string_view key) const override;
    void setStringList(std::string_view key, const std::vector<std::string>& list) override;

    bool getBool(std::string_view key, bool defaultValue = false) const override;
    void setBool(std::string_view key, bool value) override;

private:
    QSettings m_settings;
};

} // namespace mnemis::infrastructure::config
