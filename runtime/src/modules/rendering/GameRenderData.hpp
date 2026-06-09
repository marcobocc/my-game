#pragma once
#include <utility>
#include <vector>
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

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
    bool isOffscreen = false;
};
