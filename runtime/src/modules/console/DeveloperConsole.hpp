#pragma once
#include <functional>
#include <log4cxx/logger.h>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "ConsoleBuffer.hpp"
#include "commands/IConsoleCommand.hpp"
#include "messages/IConsoleMessage.hpp"
#include "messages/StringResponse.hpp"

class DeveloperConsole {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("DeveloperConsole");

public:
    using CommandFactory = std::function<std::unique_ptr<IConsoleCommand>()>;

    void registerCommand(const std::string& name, const CommandFactory& factory) { commandFactories_[name] = factory; }

    void subscribe(ConsoleBuffer<std::shared_ptr<IConsoleMessage>>& queue) { subscribers_.push_back(&queue); }

    void submitCommand(const std::string& input) { inputBuffer_.push(input); }

    std::vector<std::string> listCommands() const {
        std::vector<std::string> names;
        names.reserve(commandFactories_.size());
        for (const auto& [name, _]: commandFactories_)
            names.push_back(name);
        return names;
    }

    void broadcast(const std::shared_ptr<IConsoleMessage>& response) {
        for (auto* subscriber: subscribers_) {
            subscriber->push(response);
        }
    }

    void tick() {
        auto inputs = inputBuffer_.consume();
        for (const auto& input: inputs) {
            process(input);
        }
    }

private:
    void process(const std::string& input) {
        try {
            std::istringstream iss(input);
            std::string commandName;
            iss >> commandName;
            auto it = commandFactories_.find(commandName);
            if (it == commandFactories_.end()) {
                LOG4CXX_WARN(LOGGER, "Unknown command: " << commandName);
                broadcast(std::make_shared<StringResponse>("Unknown command: " + commandName));
                return;
            }
            std::unique_ptr<IConsoleCommand> command = it->second();
            std::string args = input.substr(commandName.length());
            if (!args.empty() && args[0] == ' ') args = args.substr(1);
            command->parse(args);
            auto response = command->execute();
            LOG4CXX_DEBUG(LOGGER, "Command executed: " << commandName);
            broadcast(std::move(response));
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, "Failed to process command: " << e.what());
            broadcast(std::make_shared<StringResponse>(std::string("ERROR: ") + e.what()));
        }
    }

    ConsoleBuffer<std::string> inputBuffer_;
    std::vector<ConsoleBuffer<std::shared_ptr<IConsoleMessage>>*> subscribers_;
    std::unordered_map<std::string, CommandFactory> commandFactories_;
};
