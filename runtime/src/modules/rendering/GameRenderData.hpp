#pragma once
#include <vector>
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"

struct GameRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    bool isOffscreen = false;
};
