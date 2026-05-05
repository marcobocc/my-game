#pragma once
#include <vector>
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/systems/rendering/vulkan/core/utils/structs.hpp"
#include "../../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"

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
