#pragma once
#include <string>
#include <vector>
#include "assets/AssetManager.hpp"
#include "core/GameWindow.hpp"
#include "core/objects/components/Camera.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"

class VulkanSwapchainManager;

class VulkanScenePass {
public:
    VulkanScenePass(const VulkanContext& context,
                    VulkanResourcesManager& resourcesManager,
                    AssetManager& assetManager,
                    GameWindow& window,
                    VulkanSwapchainManager& swapchainManager);
    ~VulkanScenePass();

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId);
    void record(VkCommandBuffer cmd, uint32_t imageIndex, const Camera& camera, const Transform& cameraTransform);
    const std::vector<DrawCall>& getDrawQueue() const { return drawQueue_; }

private:
    void createPerFrameUBO();
    void updatePerFrameUBO(const Camera& camera, const Transform& cameraTransform) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const;

    void transitionColorAttachment(VkCommandBuffer cmd, VkImage image) const;
    void transitionDepthAttachment(VkCommandBuffer cmd, VkImage image) const;
    void beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const;

    VulkanPerFrameUBO perFrameUBO_;
    VkDescriptorSetLayout perFrameUBOLayout_ = VK_NULL_HANDLE;
    std::vector<DrawCall> drawQueue_;

    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;
    VulkanContext& context_;
    GameWindow& window_;
    VulkanSwapchainManager& swapchainManager_;
};
