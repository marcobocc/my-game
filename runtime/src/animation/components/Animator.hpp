#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "../../core/components/IComponent.hpp"

struct Animator final : IComponent {
    std::string controllerName; // path to .animctrl asset

    // Runtime playback state — not serialized.
    std::vector<std::string> clipNames; // auto-populated from mesh
    std::string currentState;
    float currentTime = 0.0f;
    bool playing = false;

    // Crossfade transition — not serialized.
    std::string fromState;
    float fromTime = 0.0f;
    float transitionBlend = 1.0f;
    float transitionDuration = 0.2f;
    bool inTransition = false;

    void play(const std::string& stateName, float blendDuration = 0.2f) {
        if (playing && !currentState.empty() && blendDuration > 0.0f) {
            fromState = currentState;
            fromTime = currentTime;
            transitionBlend = 0.0f;
            transitionDuration = blendDuration;
            inTransition = true;
        } else {
            inTransition = false;
            transitionBlend = 1.0f;
        }
        currentState = stateName;
        currentTime = 0.0f;
        playing = true;
    }

    void pause() { playing = false; }

    void stop() {
        playing = false;
        currentTime = 0.0f;
        inTransition = false;
        transitionBlend = 1.0f;
    }

    std::string typeName() const override { return "Animator"; }

    nlohmann::json serialize() const override {
        return {{"controllerName", controllerName}, {"currentState", currentState}, {"playing", playing}};
    }

    static Animator deserialize(const nlohmann::json& j) {
        Animator a{};
        a.controllerName = j.value("controllerName", "");
        a.currentState = j.value("currentState", "");
        a.playing = j.value("playing", false);
        return a;
    }

    std::unique_ptr<IComponent> clone() const override { return std::make_unique<Animator>(*this); }
};
