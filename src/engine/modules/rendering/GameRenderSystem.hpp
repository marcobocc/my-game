#pragma once
#include "GameRenderData.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "modules/scene/EntityManager.hpp"
#include "vulkan/VulkanGameRenderer.hpp"

class GameRenderSystem {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("GameRenderSystem");

public:
    explicit GameRenderSystem(VulkanGameRenderer& renderer) : renderer_(renderer) {}

    void setActiveCamera(const Camera& camera, const Transform& cameraTransform) {
        activeCamera_ = &camera;
        activeCameraTransform_ = &cameraTransform;
    }

    void update(const EntityManager& entityManager) {
        std::vector<DrawCall> drawQueue;
        auto drawables = entityManager.query<Renderer, Transform>();
        for (auto& [entity, rendererPtr, transformPtr]: drawables) {
            if (!rendererPtr || !transformPtr) continue;
            if (!rendererPtr->enabled) continue;
            drawQueue.push_back({const_cast<Renderer&>(*rendererPtr),
                                 const_cast<Transform&>(*transformPtr),
                                 std::to_string(entity)});
        }

        const Camera& cam = getActiveCamera();
        const Transform& camTransform = getActiveCameraTransform();
        renderer_.renderFrame({cam, camTransform, drawQueue});
    }

private:
    Camera getActiveCamera() const {
        if (activeCamera_ != nullptr) return *activeCamera_;
        LOG4CXX_WARN(LOGGER, "Active camera not set for GameRenderSystem. Using default camera.");
        return Camera();
    }

    Transform getActiveCameraTransform() const {
        if (activeCameraTransform_ != nullptr) return *activeCameraTransform_;
        LOG4CXX_WARN(LOGGER, "Active camera transform not set for GameRenderSystem. Using default camera transform.");
        return Transform();
    }

    VulkanGameRenderer& renderer_;
    const Camera* activeCamera_ = nullptr;
    const Transform* activeCameraTransform_ = nullptr;
};
