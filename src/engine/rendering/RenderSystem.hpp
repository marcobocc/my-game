#pragma once
#include "assets/AssetManager.hpp"
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
        for (auto& [entity, mesh, material, transform]: drawables) {
            auto meshAsset = asset_manager_.get<MeshData>(mesh.name);
            auto shaderAsset = asset_manager_.get<Shader>(material.shaderName);
            backend_.draw(meshAsset, &material, shaderAsset, transform.getModelMatrix());
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
