#pragma once
#include <string>
#include "IConsoleMessage.hpp"

struct StringResponse : IConsoleMessage {
    explicit StringResponse(std::string text) : text_(std::move(text)) {}
    std::string serialize() const override { return text_; }

private:
    std::string text_;
};
