#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "IComponent.hpp"

struct Animator final : IComponent {
    // Runtime playback state — not serialized.
    std::vector<std::string> clipNames; // populated automatically from mesh at runtime
    std::string currentClip;
    float currentTime = 0.0f;
    float speed = 1.0f;
    bool playing = false;
    bool loop = true;

    void play(const std::string& clipName) {
        currentClip = clipName;
        currentTime = 0.0f;
        playing = true;
    }

    void pause() { playing = false; }

    void stop() {
        playing = false;
        currentTime = 0.0f;
    }

    void seek(float time) { currentTime = time; }

    std::string typeName() const override { return "Animator"; }

    nlohmann::json serialize() const override {
        return {{"currentClip", currentClip}, {"speed", speed}, {"playing", playing}, {"loop", loop}};
    }

    static Animator deserialize(const nlohmann::json& j) {
        Animator a{};
        if (j.contains("currentClip")) a.currentClip = j["currentClip"].get<std::string>();
        if (j.contains("speed")) a.speed = j["speed"].get<float>();
        if (j.contains("playing")) a.playing = j["playing"].get<bool>();
        if (j.contains("loop")) a.loop = j["loop"].get<bool>();
        return a;
    }

    std::unique_ptr<IComponent> clone() const override { return std::make_unique<Animator>(*this); }
};
