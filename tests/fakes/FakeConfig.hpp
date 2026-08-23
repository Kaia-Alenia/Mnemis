#pragma once
#include "core/IConfig.hpp"
#include <map>
#include <string>

namespace mnemis::tests {

class FakeConfig : public core::IConfig {
public:
    std::string getValue(std::string_view key, std::string_view defaultValue = "") const override {
        auto it = values.find(std::string(key));
        if (it != values.end()) {
            return it->second;
        }
        return std::string(defaultValue);
    }

    void setValue(std::string_view key, std::string_view value) override {
        values[std::string(key)] = std::string(value);
    }

    std::vector<std::string> getStringList(std::string_view key) const override {
        auto it = listValues.find(std::string(key));
        if (it != listValues.end()) {
            return it->second;
        }
        return {};
    }

    void setStringList(std::string_view key, const std::vector<std::string>& list) override {
        listValues[std::string(key)] = list;
    }

    bool getBool(std::string_view key, bool defaultValue = false) const override {
        auto it = boolValues.find(std::string(key));
        if (it != boolValues.end()) {
            return it->second;
        }
        return defaultValue;
    }

    void setBool(std::string_view key, bool value) override {
        boolValues[std::string(key)] = value;
    }

private:
    std::map<std::string, std::string> values;
    std::map<std::string, std::vector<std::string>> listValues;
    std::map<std::string, bool> boolValues;
};

} // namespace mnemis::tests
