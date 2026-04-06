#pragma once
#include "ecs/GameEntitiesManager.hpp"
#include "ecs/components/CameraComponent.hpp"
#include "ecs/components/RenderableComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    RenderSystem(GameEntitiesManager& ecs, VulkanGraphicsBackend& backend) : ecs_(ecs), backend_(backend) {}

    void update() {
        auto drawables = ecs_.query<RenderableComponent, TransformComponent>();
        for (auto& [entity, renderable, transform]: drawables) {
            backend_.draw(*renderable.mesh, *renderable.material, transform.getModelMatrix());
        }

        auto cameras = ecs_.query<CameraComponent>();
        if (!cameras.empty()) {
            const auto& [camEntity, camera] = cameras.front();
            backend_.renderFrame(camera);
        } else {
            throw std::runtime_error("No camera found");
        }
    }

private:
    GameEntitiesManager& ecs_;
    VulkanGraphicsBackend& backend_;
};
