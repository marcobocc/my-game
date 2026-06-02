#pragma once
#include <string>

struct IConsoleMessage {
    virtual ~IConsoleMessage() = default;
    virtual std::string serialize() const = 0;

protected:
    IConsoleMessage() = default;
};
