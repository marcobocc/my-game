#pragma once
#include "core/AssetManager.hpp"
#include "core/GameEntitiesManager.hpp"
#include "core/components/CameraComponent.hpp"
#include "core/components/RenderableComponent.hpp"
#include "core/components/TransformComponent.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    RenderSystem(GameEntitiesManager& ecs, AssetManager& asset_manager, VulkanGraphicsBackend& backend) :
        ecs_(ecs),
        asset_manager_(asset_manager),
        backend_(backend) {}

    void update() {
        auto drawables = ecs_.query<RenderableComponent, TransformComponent>();
        for (auto& [entity, renderable, transform]: drawables) {
            auto mesh = asset_manager_.getMesh(renderable.meshName);
            auto material = asset_manager_.getMaterial(renderable.materialName);
            auto shaderPipeline = asset_manager_.getShader(material->shaderPipelineName);
            backend_.draw(mesh, material, shaderPipeline, transform.getModelMatrix());
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
    AssetManager& asset_manager_;
    VulkanGraphicsBackend& backend_;
};
