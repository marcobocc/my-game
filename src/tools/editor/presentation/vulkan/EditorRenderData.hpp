#pragma once
#include <vector>
#include "../../../../engine/modules/rendering/vulkan/core/utils/structs.hpp"
#include "../../../../engine/modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../../engine/structs/components/Camera.hpp"
#include "../../../../engine/structs/components/Transform.hpp"

struct EditorRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    const std::vector<DrawCall>& outlineQueue;
    const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoLines;
    float gridScale;
    bool isOffscreen = false;

    EditorRenderData(const Camera& cam,
                     const Transform& cameraTransform,
                     const std::vector<DrawCall>& drawQ,
                     const std::vector<DrawCall>& outlineQ,
                     const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoL,
                     float gridS,
                     bool offscreen = false) :
        camera(cam),
        cameraTransform(cameraTransform),
        drawQueue(drawQ),
        outlineQueue(outlineQ),
        gizmoLines(gizmoL),
        gridScale(gridS),
        isOffscreen(offscreen) {}
};
