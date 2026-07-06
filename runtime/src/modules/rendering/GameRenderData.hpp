#pragma once
#include <glm/glm.hpp>
#include <string>
#include <utility>
#include <vector>
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/TextComponent.hpp"
#include "modules/scene/components/Transform.hpp"

struct TextDrawCall {
    std::string text;
    std::string fontName;
    glm::vec3 worldPosition{};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float fontSize = 1.0f;
    bool billboard = false;
    TextAlignment alignment = TextAlignment::Left;
};

class ParticleEmitter;

struct ParticleEmitterRef {
    ParticleEmitter* emitter = nullptr;
    glm::vec3 worldPosition = {};
    uint32_t toSpawn = 0;
};

struct GameRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    const std::vector<std::pair<Light, Transform>>& lightsWithTransforms;
    std::vector<ParticleEmitterRef> particleEmitters;
    std::vector<TextDrawCall> textQueue;
    bool isOffscreen = false;
};
