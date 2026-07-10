#pragma once
#include <array>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include "../../core/components/IComponent.hpp"

static constexpr int PARTICLE_GRADIENT_MAX_STOPS = 8;

struct ParticleColorStop {
    float time = 0.0f;
    glm::vec4 color = {1.0f, 1.0f, 1.0f, 1.0f};
};

class ParticleEmitter final : public IComponent {
public:
    float emissionRate = 20.0f;
    float lifetimeMin = 1.0f;
    float lifetimeMax = 2.0f;
    float sizeStart = 0.15f;
    float sizeEnd = 0.0f;

    // Cone shape: particles spawn in a cone pointing up (+Y world)
    float coneAngle = 25.0f; // half-angle in degrees
    float coneRadius = 0.0f; // radial spawn offset from emitter center
    float speedMin = 1.0f;
    float speedMax = 3.0f;

    glm::vec3 gravity = {0.0f, -2.0f, 0.0f};

    // Per-particle color picked randomly in [min, max] at spawn
    glm::vec4 colorStartMin = {1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 colorStartMax = {1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 colorEndMin = {1.0f, 1.0f, 1.0f, 0.0f};
    glm::vec4 colorEndMax = {1.0f, 1.0f, 1.0f, 0.0f};

    // Initial rotation in degrees (random in [min, max])
    float rotationMin = 0.0f;
    float rotationMax = 0.0f;

    // Color over lifetime gradient (multiplicative with per-particle color)
    // Always has stops at t=0 and t=1; colorKeyCount in [2, PARTICLE_GRADIENT_MAX_STOPS]
    int colorKeyCount = 2;
    std::array<ParticleColorStop, PARTICLE_GRADIENT_MAX_STOPS> colorKeys = {{
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {1.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
            {0.0f, {1.0f, 1.0f, 1.0f, 1.0f}},
    }};

    uint32_t maxParticles = 2048;
    std::string spriteTexture;

    float spawnAccumulator = 0.0f;

    std::string typeName() const override { return "ParticleEmitter"; }

    nlohmann::json serialize() const override {
        nlohmann::json j;
        j["emissionRate"] = emissionRate;
        j["lifetimeMin"] = lifetimeMin;
        j["lifetimeMax"] = lifetimeMax;
        j["sizeStart"] = sizeStart;
        j["sizeEnd"] = sizeEnd;
        j["coneAngle"] = coneAngle;
        j["coneRadius"] = coneRadius;
        j["speedMin"] = speedMin;
        j["speedMax"] = speedMax;
        j["gravity"] = {gravity.x, gravity.y, gravity.z};
        j["colorStartMin"] = {colorStartMin.r, colorStartMin.g, colorStartMin.b, colorStartMin.a};
        j["colorStartMax"] = {colorStartMax.r, colorStartMax.g, colorStartMax.b, colorStartMax.a};
        j["colorEndMin"] = {colorEndMin.r, colorEndMin.g, colorEndMin.b, colorEndMin.a};
        j["colorEndMax"] = {colorEndMax.r, colorEndMax.g, colorEndMax.b, colorEndMax.a};
        j["rotationMin"] = rotationMin;
        j["rotationMax"] = rotationMax;
        j["maxParticles"] = maxParticles;
        if (!spriteTexture.empty()) j["spriteTexture"] = spriteTexture;
        j["colorKeyCount"] = colorKeyCount;
        auto& keys = j["colorKeys"] = nlohmann::json::array();
        for (int i = 0; i < colorKeyCount; ++i) {
            keys.push_back(
                    {{"time", colorKeys[i].time},
                     {"color",
                      {colorKeys[i].color.r, colorKeys[i].color.g, colorKeys[i].color.b, colorKeys[i].color.a}}});
        }
        return j;
    }

    std::unique_ptr<IComponent> clone() const override { return std::make_unique<ParticleEmitter>(*this); }

    static ParticleEmitter deserialize(const nlohmann::json& j) {
        ParticleEmitter e;
        e.emissionRate = j.value("emissionRate", e.emissionRate);
        e.lifetimeMin = j.value("lifetimeMin", e.lifetimeMin);
        e.lifetimeMax = j.value("lifetimeMax", e.lifetimeMax);
        e.sizeStart = j.value("sizeStart", e.sizeStart);
        e.sizeEnd = j.value("sizeEnd", e.sizeEnd);
        e.coneAngle = j.value("coneAngle", e.coneAngle);
        e.coneRadius = j.value("coneRadius", e.coneRadius);
        e.speedMin = j.value("speedMin", e.speedMin);
        e.speedMax = j.value("speedMax", e.speedMax);
        e.maxParticles = j.value("maxParticles", e.maxParticles);
        e.spriteTexture = j.value("spriteTexture", std::string{});
        e.rotationMin = j.value("rotationMin", e.rotationMin);
        e.rotationMax = j.value("rotationMax", e.rotationMax);
        e.colorKeyCount = j.value("colorKeyCount", e.colorKeyCount);
        if (j.contains("gravity")) {
            const auto& v = j["gravity"];
            e.gravity = {v[0], v[1], v[2]};
        }
        auto readVec4 = [&](const char* key, glm::vec4 def) -> glm::vec4 {
            if (!j.contains(key)) return def;
            const auto& v = j[key];
            return {v[0], v[1], v[2], v[3]};
        };
        e.colorStartMin = readVec4("colorStartMin", e.colorStartMin);
        e.colorStartMax = readVec4("colorStartMax", e.colorStartMax);
        e.colorEndMin = readVec4("colorEndMin", e.colorEndMin);
        e.colorEndMax = readVec4("colorEndMax", e.colorEndMax);
        if (j.contains("colorKeys")) {
            const auto& arr = j["colorKeys"];
            e.colorKeyCount = std::min((int) arr.size(), PARTICLE_GRADIENT_MAX_STOPS);
            for (int i = 0; i < e.colorKeyCount; ++i) {
                e.colorKeys[i].time = arr[i].value("time", 0.0f);
                const auto& c = arr[i]["color"];
                e.colorKeys[i].color = {c[0], c[1], c[2], c[3]};
            }
        }
        return e;
    }
};
