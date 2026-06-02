#pragma once
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "messages/IConsoleMessage.hpp"
#include "modules/console/ConsoleBuffer.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/console/messages/LogMessage.hpp"

class Imgui_Console {
public:
    explicit Imgui_Console(DeveloperConsole& console) : console_(console) { console_.subscribe(responseBuffer_); }

    void draw();
    void show() { visible_ = true; }
    void hide() { visible_ = false; }
    void toggleVisibility() { visible_ = !visible_; }
    bool isVisible() const { return visible_; }
    void setLevelFilter(LogLevel level, bool enabled);
    void setSearchFilter(const std::string& search);
    const std::string& getLastCommand() const { return lastCommand_; }
    void clearLastCommand() { lastCommand_.clear(); }

private:
    bool matchesFilters(const LogMessage& msg) const;

    bool visible_ = false;
    DeveloperConsole& console_;
    ConsoleBuffer<std::shared_ptr<IConsoleMessage>> responseBuffer_;
    std::deque<std::shared_ptr<IConsoleMessage>> cachedMessages_;
    std::set<LogLevel> enabledLevels_ = {LogLevel::Debug, LogLevel::Info, LogLevel::Warn, LogLevel::Error};
    std::string searchFilter_;
    std::string lastCommand_;
    std::vector<std::string> matchedCommands_;
};
