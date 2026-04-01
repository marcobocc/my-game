#pragma once
#include "ecs/GameEntitiesManager.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    RenderSystem(GameEntitiesManager& ecs, VulkanGraphicsBackend& backend) : ecs_(ecs), backend_(backend) {}

    void update() const {
        auto drawables = ecs_.query<MeshComponent, MaterialComponent>();
        for (auto& [entity, mesh, material]: drawables) {
            backend_.draw(mesh, material);
        }
        backend_.renderFrame();
    }

private:
    GameEntitiesManager& ecs_;
    VulkanGraphicsBackend& backend_;
};
