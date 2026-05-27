#pragma once
#include <nlohmann/json.hpp>
#include <optional>

enum class ClipboardPayloadType { Entity };

class ClipboardService {
public:
    void copy(const nlohmann::json& data, ClipboardPayloadType type) {
        clipboard_ = data;
        payloadType_ = type;
    }

    void clear() {
        clipboard_.reset();
        payloadType_.reset();
    }

    const std::optional<nlohmann::json>& get() const { return clipboard_; }

    bool hasData() const { return clipboard_.has_value() && payloadType_.has_value(); }

    bool hasEntity() const { return payloadType_.has_value() && *payloadType_ == ClipboardPayloadType::Entity; }

private:
    std::optional<nlohmann::json> clipboard_;
    std::optional<ClipboardPayloadType> payloadType_;
};
