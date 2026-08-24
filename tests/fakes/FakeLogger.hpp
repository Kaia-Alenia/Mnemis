#pragma once
#include "core/ILogger.hpp"
#include <vector>
#include <string>

#include <iostream>

namespace mnemis::tests {

class FakeLogger : public core::ILogger {
public:
    void log(core::LogLevel level, std::string_view message) override {
        logs.push_back({level, std::string(message)});
        std::cout << "[LOG] " << message << std::endl;
    }

    struct LogEntry {
        core::LogLevel level;
        std::string message;
    };

    std::vector<LogEntry> logs;
};

} // namespace mnemis::tests
