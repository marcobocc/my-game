#pragma once
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "assets/AssetManager.hpp"
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Mesh.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanContext.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanPipeline.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanTexture.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanTextureSet.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanUBO.hpp"
#include "rendering/vulkan/services/VulkanResourceCache.hpp"

struct DrawCall {
    Mesh mesh;
    Material material;
    Transform transform;
};

class VulkanSceneRenderer {
public:
    VulkanSceneRenderer(const VulkanContext& vulkanContext,
                        VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                        VulkanResourceCache<VulkanPipeline>& pipelineCache,
                        VulkanResourceCache<VulkanTexture>& textureCache,
                        VulkanSwapchain& swapchain,
                        VulkanUBO& cameraUBO,
                        VulkanTextureSet* textureSet,
                        AssetManager& assetManager);

    void recordScenePass(VkCommandBuffer cmd,
                         uint32_t imageIndex,
                         size_t currentFrame,
                         const std::vector<DrawCall>& drawCalls);

private:
    void beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void endRendering(VkCommandBuffer cmd) const;
    void setupViewportAndScissor(VkCommandBuffer cmd) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, size_t currentFrame);

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VulkanResourceCache<VulkanBuffer>& vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline>& pipelineCache_;
    VulkanResourceCache<VulkanTexture>& textureCache_;
    VulkanSwapchain& swapchain_;
    VulkanUBO& cameraUBO_;
    AssetManager& assetManager_;
    VulkanTextureSet* textureSet_ = nullptr;
    std::unordered_map<std::string, VkDescriptorSet> textureDescriptorSets_;
};
