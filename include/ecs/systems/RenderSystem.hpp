#pragma once
#include "ecs/GameEntitiesManager.hpp"
#include "ecs/components/CameraComponent.hpp"
#include "ecs/components/TransformComponent.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    RenderSystem(GameEntitiesManager& ecs, VulkanGraphicsBackend& backend) : ecs_(ecs), backend_(backend) {}

    void update() {
        auto drawables = ecs_.query<MeshComponent, MaterialComponent, TransformComponent>();
        for (auto& [entity, mesh, material, transform]: drawables) {
            backend_.draw(mesh, material, transform.getModelMatrix());
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
