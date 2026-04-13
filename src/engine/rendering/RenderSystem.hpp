#pragma once
#include "core/assets/AssetManager.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Mesh.hpp"
#include "core/objects/components/Transform.hpp"
#include "core/scene/Scene.hpp"
#include "rendering/vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    RenderSystem(AssetManager& asset_manager, VulkanGraphicsBackend& backend) :
        asset_manager_(asset_manager),
        backend_(backend) {}

    void update(const Scene& scene) {
        auto drawables = scene.getObjectsWith<Mesh, Material, Transform>();
        for (auto& [entity, meshRef, material, transform]: drawables) {
            auto meshPtr = asset_manager_.getMesh(meshRef.name);
            auto shaderPipeline = asset_manager_.getShader(material.shaderName);
            backend_.draw(meshPtr, &material, shaderPipeline, transform.getModelMatrix());
        }

        auto cameras = scene.getObjectsWith<Camera>();
        if (!cameras.empty()) {
            const auto& [camEntity, camera] = cameras.front();
            backend_.renderFrame(camera);
        } else {
            throw std::runtime_error("No camera found");
        }
    }

private:
    AssetManager& asset_manager_;
    VulkanGraphicsBackend& backend_;
};
