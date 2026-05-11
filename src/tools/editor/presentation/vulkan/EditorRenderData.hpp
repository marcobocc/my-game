#pragma once
#include <utility>
#include <vector>
#include "../../../../engine/modules/rendering/vulkan/core/utils/structs.hpp"
#include "../../../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../../engine/modules/scene/components/Camera.hpp"
#include "../../../../engine/modules/scene/components/Light.hpp"
#include "../../../../engine/modules/scene/components/Transform.hpp"

struct EditorRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    const std::vector<DrawCall>& outlineQueue;
    const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoLines;
    const std::vector<std::pair<Light, Transform>>& lightsWithTransforms;
    float gridScale;
    bool isOffscreen = false;

    EditorRenderData(const Camera& cam,
                     const Transform& cameraTransform,
                     const std::vector<DrawCall>& drawQ,
                     const std::vector<DrawCall>& outlineQ,
                     const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoL,
                     const std::vector<std::pair<Light, Transform>>& lightsWT,
                     float gridS,
                     bool offscreen = false) :
        camera(cam),
        cameraTransform(cameraTransform),
        drawQueue(drawQ),
        outlineQueue(outlineQ),
        gizmoLines(gizmoL),
        lightsWithTransforms(lightsWT),
        gridScale(gridS),
        isOffscreen(offscreen) {}
};
