#pragma once
#include "core/GameEntitiesManager.hpp"
#include "core/components/CameraComponent.hpp"
#include "core/components/RenderableComponent.hpp"
#include "core/components/TransformComponent.hpp"
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
