#pragma once
#include <utility>
#include <vector>
#include "modules/rendering/GameRenderData.hpp"
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "modules/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

struct EditorRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    const std::vector<DrawCall>& outlineQueue;
    const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoLines;
    const std::vector<VulkanGizmoPass::GizmoVertex>& overlayGizmoLines;
    const std::vector<std::pair<Light, Transform>>& lightsWithTransforms;
    std::vector<ParticleEmitterRef> particleEmitters;
    float gridScale;
    bool isOffscreen = false;

    EditorRenderData(const Camera& cam,
                     const Transform& cameraTransform,
                     const std::vector<DrawCall>& drawQ,
                     const std::vector<DrawCall>& outlineQ,
                     const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoL,
                     const std::vector<VulkanGizmoPass::GizmoVertex>& overlayGizmoL,
                     const std::vector<std::pair<Light, Transform>>& lightsWT,
                     std::vector<ParticleEmitterRef> particlesIn,
                     float gridS,
                     bool offscreen = false) :
        camera(cam),
        cameraTransform(cameraTransform),
        drawQueue(drawQ),
        outlineQueue(outlineQ),
        gizmoLines(gizmoL),
        overlayGizmoLines(overlayGizmoL),
        lightsWithTransforms(lightsWT),
        particleEmitters(std::move(particlesIn)),
        gridScale(gridS),
        isOffscreen(offscreen) {}
};
