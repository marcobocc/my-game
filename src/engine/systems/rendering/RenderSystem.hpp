#pragma once
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/scene/Scene.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("RenderSystem");

public:
    explicit RenderSystem(VulkanGraphicsBackend& backend) : backend_(backend) {}

    void setActiveCamera(const Camera& camera, const Transform& cameraTransform) {
        activeCamera_ = &camera;
        activeCameraTransform_ = &cameraTransform;
    }

    void update(const Scene& scene) {
        auto drawables = scene.getObjectsWith<Renderer, Transform>();
        for (auto& [entity, renderer, transform]: drawables) {
            if (!renderer.enabled) continue;
            backend_.draw(renderer, transform, entity);
        }
        backend_.renderFrame(getActiveCamera(), getActiveCameraTransform());
    }

private:
    Camera getActiveCamera() const {
        if (activeCamera_ != nullptr) return *activeCamera_;
        LOG4CXX_WARN(LOGGER, "Active camera not set for RenderSystem. Using default camera.");
        return Camera();
    }

    Transform getActiveCameraTransform() const {
        if (activeCamera_ != nullptr) return *activeCameraTransform_;
        LOG4CXX_WARN(LOGGER, "Active camera transform not set for RenderSystem. Using default camera transform.");
        return Transform();
    }

    VulkanGraphicsBackend& backend_;
    const Camera* activeCamera_ = nullptr;
    const Transform* activeCameraTransform_ = nullptr;
};
