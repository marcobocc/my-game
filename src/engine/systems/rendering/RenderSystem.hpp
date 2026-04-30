#pragma once
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "systems/scene/Scene.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class RenderSystem {
public:
    explicit RenderSystem(VulkanGraphicsBackend& backend) : backend_(backend) {}

    void update(const Scene& scene) {
        auto drawables = scene.getObjectsWith<Renderer, Transform>();
        for (auto& [entity, renderer, transform]: drawables) {
            if (!renderer.enabled) continue;
            backend_.draw(renderer, transform, entity);
        }

        auto cameras = scene.getObjectsWith<Camera, Transform>();
        if (!cameras.empty()) {
            const auto& [camEntity, camera, transform] = cameras.front();
            backend_.renderFrame(camera, transform);
        } else {
            throw std::runtime_error("No camera found");
        }
    }

private:
    VulkanGraphicsBackend& backend_;
};
