#pragma once
#include <argparse/argparse.hpp>
#include <memory>
#include <string>
#include <vector>
#include "../messages/IConsoleMessage.hpp"


struct IConsoleCommand {
    virtual ~IConsoleCommand() = default;
    virtual std::unique_ptr<IConsoleMessage> execute() = 0;
    virtual std::string cmdName() const = 0;
    virtual void parse(const std::string& args) = 0;

protected:
    IConsoleCommand() = default;

    static std::vector<std::string> splitArgs(const std::string& args) {
        std::vector<std::string> result;
        result.push_back("");
        std::string current;
        for (char c: args) {
            if (c == ' ') {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            result.push_back(current);
        }
        return result;
    }
};
