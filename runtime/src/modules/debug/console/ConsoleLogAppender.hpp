#pragma once
#include <log4cxx/appenderskeleton.h>
#include <log4cxx/level.h>
#include <log4cxx/spi/loggingevent.h>
#include "modules/debug/console/DeveloperConsole.hpp"
#include "modules/debug/console/messages/LogMessage.hpp"

class ConsoleLogAppender : public log4cxx::AppenderSkeleton {
public:
    explicit ConsoleLogAppender(DeveloperConsole* console) : console_(console) {}

    void append(const log4cxx::spi::LoggingEventPtr& event, log4cxx::helpers::Pool& p) override {
        if (!console_) return;
        if (getLayout()) {
            log4cxx::LogString output = event->getRenderedMessage();
            auto level = LogLevel::Info;
            if (event->getLevel() == log4cxx::Level::getDebug())
                level = LogLevel::Debug;
            else if (event->getLevel() == log4cxx::Level::getWarn())
                level = LogLevel::Warn;
            else if (event->getLevel() == log4cxx::Level::getError() || event->getLevel() == log4cxx::Level::getFatal())
                level = LogLevel::Error;

            auto timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds(event->getTimeStamp()));
            auto msg = std::make_shared<LogMessage>();
            msg->timestamp = timestamp;
            msg->level = level;
            msg->source = event->getLoggerName();
            msg->text = std::string(output);
            console_->broadcast(std::move(msg));
        }
    }

    void close() override {}
    bool requiresLayout() const override { return true; }

private:
    DeveloperConsole* console_;
};
