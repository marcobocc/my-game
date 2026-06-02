#pragma once
#include <string>
#include "IComponent.hpp"

struct BehaviourScript final : IComponent {
    std::string scriptName;

    BehaviourScript() = default;
    explicit BehaviourScript(std::string name) : scriptName(std::move(name)) {}

    std::string typeName() const override { return "BehaviourScript"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<BehaviourScript>(*this); }
    bool allowMultiple() const override { return true; }

    nlohmann::json serialize() const override { return {{"scriptName", scriptName}}; }

    static BehaviourScript deserialize(const nlohmann::json& j) {
        BehaviourScript b{};
        b.scriptName = j.at("scriptName").get<std::string>();
        return b;
    }
};
