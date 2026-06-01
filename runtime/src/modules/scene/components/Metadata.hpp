#pragma once
#include <string>
#include "../../../utils/JsonUtils.hpp"
#include "IComponent.hpp"

struct Metadata final : IComponent {
    std::string displayName;

    Metadata() = default;
    explicit Metadata(const std::string& name) : displayName(name) {}

    nlohmann::json serialize() const override { return {{"displayName", displayName}}; }

    std::string typeName() const override { return "Metadata"; }

    static Metadata deserialize(const nlohmann::json& j) {
        Metadata m{};
        m.displayName = JsonUtils::getRequired<std::string>(j, "displayName");
        return m;
    }
};
