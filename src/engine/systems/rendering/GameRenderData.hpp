#pragma once
#include <vector>
#include "data/components/Camera.hpp"
#include "data/components/Transform.hpp"
#include "systems/rendering/vulkan/core/utils/structs.hpp"

struct GameRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    bool isOffscreen = false;
};
