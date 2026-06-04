#pragma once
#include <utility>
#include <vector>
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

struct GameRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    const std::vector<std::pair<Light, Transform>>& lightsWithTransforms;
    bool isOffscreen = false;
};
