#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include "../../core/assets/Asset.hpp"

struct AnimatorControllerState {
    std::string clipName;
    float speed = 1.0f;
    bool loop = true;
};

class AnimatorController final : public Asset {
public:
    AnimatorController(const std::string& name, std::unordered_map<std::string, AnimatorControllerState> states) :
        Asset(name),
        states(std::move(states)) {}

    static AnimatorController deserialize(const nlohmann::json& j, const std::string& name) {
        std::unordered_map<std::string, AnimatorControllerState> states;
        if (j.contains("states")) {
            for (const auto& [stateName, s]: j["states"].items()) {
                AnimatorControllerState state;
                state.clipName = s.value("clipName", "");
                state.speed = s.value("speed", 1.0f);
                state.loop = s.value("loop", true);
                states[stateName] = state;
            }
        }
        return AnimatorController{name, std::move(states)};
    }

    nlohmann::json serialize() const {
        nlohmann::json statesJson = nlohmann::json::object();
        for (const auto& [name, state]: states)
            statesJson[name] = {{"clipName", state.clipName}, {"speed", state.speed}, {"loop", state.loop}};
        return {{"states", statesJson}};
    }

    std::unordered_map<std::string, AnimatorControllerState> states;
};
