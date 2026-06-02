#pragma once
#include <argparse/argparse.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include "IConsoleCommand.hpp"
#include "modules/console/messages/StringResponse.hpp"


class EchoCommand : public IConsoleCommand {
public:
    EchoCommand() = default;
    ~EchoCommand() override = default;

    void parse(const std::string& args) override {
        auto program = argparse::ArgumentParser(cmdName());
        program.add_argument("text").help("Text to echo");
        program.add_argument("-n", "--number").help("Number of times to echo").default_value(1).scan<'i', int>();
        try {
            program.parse_args(splitArgs(args));
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to parse " + cmdName() + " command: " + std::string(e.what()));
        }
        text_ = program.get<std::string>("text");
        count_ = program.get<int>("-n");
    }

    std::unique_ptr<IConsoleMessage> execute() override {
        std::string result;
        for (int i = 0; i < count_; ++i) {
            result += text_;
            if (i < count_ - 1) result += "\n";
        }
        return std::make_unique<StringResponse>(std::move(result));
    }

    std::string cmdName() const override { return "echo"; }

private:
    std::string text_;
    int count_ = 1;
};
