#pragma once
#include <glm/glm.hpp>
#include <string>
#include <utility>
#include <vector>
#include "../core/components/Transform.hpp"
#include "components/Camera.hpp"
#include "components/Light.hpp"
#include "components/TextComponent.hpp"
#include "graphics/vulkan/core/utils/structs.hpp"

struct TextDrawCall {
    std::string text;
    std::string fontName;
    glm::vec3 worldPosition{};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float fontSize = 1.0f;
    bool billboard = false;
    TextAlignment alignment = TextAlignment::Left;
};

struct UIVertex {
    glm::vec3 position; // xy in logical scene-viewport pixels, z = 0
    glm::vec2 uv;
    glm::vec4 color;
};

struct UIDrawCall {
    enum class Kind { Quads, Text };
    Kind kind = Kind::Quads;
    std::string textureOrFont; // Quads: texture name (empty = white); Text: font name
    std::vector<UIVertex> vertices; // Quads only
    std::string text; // Text only
    glm::vec2 position{}; // Text baseline origin, logical pixels
    float fontSize = 16.0f;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
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
    std::vector<UIDrawCall> uiQueue;
    bool isOffscreen = false;
};
