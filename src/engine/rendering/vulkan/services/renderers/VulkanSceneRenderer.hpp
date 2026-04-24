#pragma once
#include <string>
#include <vector>
#include "assets/AssetManager.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"

struct DrawCall {
    const Renderer& renderer;
    const Transform& transform;
    std::string objectId;
};

class VulkanSceneRenderer {
public:
    VulkanSceneRenderer(const VulkanContext& vulkanContext,
                        VulkanResourcesManager& resourcesManager,
                        AssetManager& assetManager);

    ~VulkanSceneRenderer();

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId);
    void drawScene(VkCommandBuffer cmd, const Camera& camera, const Transform& cameraTransform);
    const std::vector<DrawCall>& getDrawQueue() const { return drawQueue_; }

private:
    void createPerFrameUBO();
    void updatePerFrameUBO(const Camera& camera, const Transform& cameraTransform) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const;

    VulkanPerFrameUBO perFrameUBO_;
    VkDescriptorSetLayout perFrameUBOLayout_ = VK_NULL_HANDLE;
    std::vector<DrawCall> drawQueue_;

    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;
    VulkanContext& vulkanContext_;
};
