#pragma once
#include "assets/AssetManager.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Mesh.hpp"
#include "core/objects/components/RenderProperties.hpp"
#include "core/objects/components/Transform.hpp"
#include "core/scene/Scene.hpp"
#include "rendering/vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    explicit RenderSystem(VulkanGraphicsBackend& backend) : backend_(backend) {}

    void update(const Scene& scene) {
        auto drawables = scene.getObjectsWith<Mesh, Material, Transform, RenderProperties>();
        for (auto& [entity, mesh, material, transform, renderProps]: drawables) {
            if (!renderProps.enabled) continue;
            backend_.draw(mesh, material, transform);
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
    VulkanGraphicsBackend& backend_;
};
