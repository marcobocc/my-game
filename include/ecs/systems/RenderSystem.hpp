#pragma once
#include "ecs/GameEntitiesManager.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"
#include "ecs/components/TransformComponent.hpp"

class RenderSystem {
public:
    RenderSystem(GameEntitiesManager& ecs, VulkanGraphicsBackend& backend) : ecs_(ecs), backend_(backend) {}

    void update() const {
        auto drawables = ecs_.query<MeshComponent, MaterialComponent, TransformComponent>();
        for (auto& [entity, mesh, material, transform]: drawables) {
            backend_.draw(mesh, material, transform.getModelMatrix());
        }
        backend_.renderFrame();
    }

private:
    GameEntitiesManager& ecs_;
    VulkanGraphicsBackend& backend_;
};
