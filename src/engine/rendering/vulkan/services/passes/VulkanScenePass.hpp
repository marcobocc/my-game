#pragma once
#include <string>
#include <vector>
#include "assets/AssetManager.hpp"
#include "core/GameWindow.hpp"
#include "core/objects/components/Camera.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"

class VulkanScenePass {
public:
    VulkanScenePass(const VulkanContext& context,
                    VulkanResourcesManager& resourcesManager,
                    AssetManager& assetManager,
                    GameWindow& window);
    ~VulkanScenePass();

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId);
    void record(VkCommandBuffer cmd,
                VkImageView colorView,
                VkImageView depthView,
                VkExtent2D extent,
                const Camera& camera,
                const Transform& cameraTransform);
    const std::vector<DrawCall>& getDrawQueue() const { return drawQueue_; }

private:
    void createPerFrameUBO();
    void updatePerFrameUBO(const Camera& camera, const Transform& cameraTransform) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const;

    void beginRendering(VkCommandBuffer cmd, VkImageView colorView, VkImageView depthView, VkExtent2D extent) const;

    VulkanPerFrameUBO perFrameUBO_;
    VkDescriptorSetLayout perFrameUBOLayout_ = VK_NULL_HANDLE;
    std::vector<DrawCall> drawQueue_;

    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;
    VulkanContext& context_;
    GameWindow& window_;
};
