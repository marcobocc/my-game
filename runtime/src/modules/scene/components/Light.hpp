#pragma once
#include <glm/vec3.hpp>
#include <nlohmann/json.hpp>
#include "IComponent.hpp"

enum class LightType { DIRECTIONAL = 0, SPOT = 1 };

class Light final : public IComponent {
public:
    explicit Light(LightType type = LightType::DIRECTIONAL, float intensity = 1.0f) :
        type(type),
        intensity(intensity) {}

    LightType type;
    float intensity;

    // Spotlight-only. Cone half-angles in degrees; the cone falls off between inner and outer.
    // range is the distance at which the light fully attenuates.
    float innerConeAngle = 20.0f;
    float outerConeAngle = 30.0f;
    float range = 25.0f;

    bool castShadows = true;

    nlohmann::json serialize() const override {
        nlohmann::json j;
        j["type"] = static_cast<int>(type);
        j["intensity"] = intensity;
        j["innerConeAngle"] = innerConeAngle;
        j["outerConeAngle"] = outerConeAngle;
        j["range"] = range;
        j["castShadows"] = castShadows;
        return j;
    }

    std::string typeName() const override { return "Light"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<Light>(*this); }

    static Light deserialize(const nlohmann::json& j) {
        LightType type = static_cast<LightType>(j.at("type").get<int>());
        float intensity = j.at("intensity").get<float>();
        Light light(type, intensity);
        light.innerConeAngle = j.value("innerConeAngle", light.innerConeAngle);
        light.outerConeAngle = j.value("outerConeAngle", light.outerConeAngle);
        light.range = j.value("range", light.range);
        light.castShadows = j.value("castShadows", light.castShadows);
        return light;
    }
};
