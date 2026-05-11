#pragma once
#include <vector>
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "structs/components/Camera.hpp"
#include "structs/components/Transform.hpp"

struct GameRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    bool isOffscreen = false;
};
