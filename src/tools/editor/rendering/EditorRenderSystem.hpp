#pragma once
#include "../../../engine/data/components/Camera.hpp"
#include "../../../engine/data/components/Renderer.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../engine/systems/scene/Scene.hpp"
#include "EditorRenderData.hpp"
#include "VulkanEditorRenderer.hpp"

class EditorRenderSystem {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("EditorRenderSystem");

public:
    explicit EditorRenderSystem(VulkanEditorRenderer& renderer) : renderer_(renderer) {}

    void setActiveCamera(const Camera& camera, const Transform& cameraTransform) {
        activeCamera_ = &camera;
        activeCameraTransform_ = &cameraTransform;
    }

    void update(const Scene& scene,
                const std::vector<DrawCall>& outlineQueue = {},
                const std::vector<VulkanGizmoPass::GizmoVertex>& gizmoLines = {}) {

        std::vector<DrawCall> drawQueue;
        auto drawables = scene.getObjectsWith<Renderer, Transform>();
        for (auto& [entity, renderer, transform]: drawables) {
            if (!renderer.enabled) continue;
            drawQueue.push_back({renderer, transform, entity});
        }

        auto cameras = scene.getObjectsWith<Camera, Transform>();
        for (auto& [entity, camera, transform]: cameras) {
            if (camera.renderTarget.isValid())
                renderer_.renderFrame({camera, transform, drawQueue, outlineQueue, gizmoLines, true});
        }

        const Camera& cam = getActiveCamera();
        const Transform& camTransform = getActiveCameraTransform();
        renderer_.renderFrame({cam, camTransform, drawQueue, outlineQueue, gizmoLines});
    }

private:
    Camera getActiveCamera() const {
        if (activeCamera_ != nullptr) return *activeCamera_;
        LOG4CXX_WARN(LOGGER, "Active camera not set for EditorRenderSystem. Using default camera.");
        return Camera();
    }

    Transform getActiveCameraTransform() const {
        if (activeCamera_ != nullptr) return *activeCameraTransform_;
        LOG4CXX_WARN(LOGGER, "Active camera transform not set for EditorRenderSystem. Using default camera transform.");
        return Transform();
    }

    VulkanEditorRenderer& renderer_;
    const Camera* activeCamera_ = nullptr;
    const Transform* activeCameraTransform_ = nullptr;
};
