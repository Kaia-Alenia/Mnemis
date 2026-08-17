#pragma once
#include "core/IConfig.hpp"
#include <map>
#include <string>

namespace mnemis::tests {

class FakeConfig : public core::IConfig {
public:
    std::string getValue(std::string_view key) const override {
        auto it = values.find(std::string(key));
        if (it != values.end()) {
            return it->second;
        }
        return "";
    }

    void setValue(std::string key, std::string value) {
        values[key] = value;
    }

private:
    std::map<std::string, std::string> values;
};

} // namespace mnemis::tests
