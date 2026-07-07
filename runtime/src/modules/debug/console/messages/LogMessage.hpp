#pragma once
#include <chrono>
#include <string>
#include "IConsoleMessage.hpp"

enum class LogLevel { Debug, Info, Warn, Error };

struct LogMessage : IConsoleMessage {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string source;
    std::string text;

    std::string serialize() const override { return text; }
};
