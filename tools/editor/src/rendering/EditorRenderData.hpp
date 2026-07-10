#pragma once
#include <utility>
#include <vector>
#include "../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../runtime/src/graphics/components/Camera.hpp"
#include "../../../../runtime/src/graphics/components/Light.hpp"
#include "graphics/GameRenderData.hpp"
#include "graphics/vulkan/core/utils/structs.hpp"
#include "graphics/vulkan/passes/VulkanGizmoPass.hpp"

struct EditorRenderData {
    const Camera& camera;
    const Transform& cameraTransform;
    const std::vector<DrawCall>& drawQueue;
    const std::vector<DrawCall>& outlineQueue;
    const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoLines;
    const std::vector<VulkanGizmoPass::GizmoVertex>& overlayGizmoLines;
    const std::vector<std::pair<Light, Transform>>& lightsWithTransforms;
    std::vector<ParticleEmitterRef> particleEmitters;
    std::vector<TextDrawCall> textQueue;
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
                     std::vector<TextDrawCall> textIn,
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
        textQueue(std::move(textIn)),
        gridScale(gridS),
        isOffscreen(offscreen) {}
};
