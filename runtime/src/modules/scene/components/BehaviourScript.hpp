#pragma once
#include <string>
#include <unordered_map>
#include "IComponent.hpp"

struct BehaviourScript final : IComponent {
    std::string scriptName;
    std::unordered_map<std::string, nlohmann::json> propertyValues;

    BehaviourScript() = default;
    explicit BehaviourScript(std::string name) : scriptName(std::move(name)) {}

    std::string typeName() const override { return "BehaviourScript"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<BehaviourScript>(*this); }
    bool allowMultiple() const override { return true; }

    nlohmann::json serialize() const override {
        nlohmann::json j;
        j["scriptName"] = scriptName;
        if (!propertyValues.empty()) j["propertyValues"] = propertyValues;
        return j;
    }

    static BehaviourScript deserialize(const nlohmann::json& j) {
        BehaviourScript b{};
        b.scriptName = j.at("scriptName").get<std::string>();
        if (j.contains("propertyValues"))
            b.propertyValues = j.at("propertyValues").get<std::unordered_map<std::string, nlohmann::json>>();
        return b;
    }
};
