#pragma once
#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>

enum class LightType { Directional = 0 };

class Light final {
public:
    explicit Light(LightType type = LightType::Directional, float intensity = 1.0f) :
        type(type),
        intensity(intensity) {}

    LightType type;
    float intensity;

    nlohmann::json serialize() const {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["intensity"] = intensity;
        return j;
    }

    static Light deserialize(const nlohmann::json& j) {
        LightType type = static_cast<LightType>(j.at("type").get<int>());
        float intensity = j.at("intensity").get<float>();
        return Light(type, intensity);
    }
};
