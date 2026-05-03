#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include "../GameRenderData.hpp"
#include "core/RenderGraph.hpp"
#include "core/utils/structs.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Transform.hpp"
#include "data/settings/RendererSettings.hpp"

class VulkanGeometryPass;
class VulkanLightingPass;
class VulkanSwapchainManager;
class VulkanFrameManager;
class VulkanRenderTargetManager;

class VulkanGameRenderer {
public:
    VulkanGameRenderer(VulkanContext& context,
                       VulkanFrameManager& frameManager,
                       VulkanRenderTargetManager& renderTargetManager,
                       VulkanGeometryPass& geometryPass,
                       VulkanLightingPass& lightingPass,
                       VulkanSwapchainManager& swapchainManager,
                       RendererSettings& settings);

    ~VulkanGameRenderer();

    // Render one frame for the given game render data.
    bool renderFrame(const GameRenderData& scene);

    RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height);
    void destroyRenderTarget(RenderTargetHandle handle);

private:
    // --- Frame helpers ---
    void recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const GameRenderData& scene);
    void executeRenderGraph(VkCommandBuffer cmd,
                            VkImage colorImage,
                            VkImageView colorView,
                            VkExtent2D extent,
                            const GameRenderData& ctx);

    // --- Render graph setup ---
    void setupRenderGraph(VkFormat colorFormat, VkImageUsageFlags colorUsage, VkExtent2D extent);

    // --- Dependencies ---
    VulkanContext& context_;
    RendererSettings& settings_;
    VulkanSwapchainManager& swapchainManager_;
    VulkanFrameManager& frameManager_;
    VulkanRenderTargetManager& renderTargetManager_;
    VulkanGeometryPass& geometryPass_;
    VulkanLightingPass& lightingPass_;

    // --- Render graph ---
    std::optional<VulkanRenderGraph<GameRenderData>> graph_;
    ResourceHandle colorTargetHandle_;
    ResourceHandle gbufferAlbedoHandle_;
    ResourceHandle gbufferNormalHandle_;
    ResourceHandle gbufferDepthHandle_;

    // --- Frame state ---
    uint32_t frameCount_ = 0;
    VkFormat swapchainColorFormat_ = VK_FORMAT_UNDEFINED;

    // --- Per-frame descriptor sets and buffers ---
    std::vector<VkDescriptorSet> mainLightingDescriptorSets_;
    std::vector<VkDescriptorSet> mainGeometryDescriptorSets_;
    std::vector<VulkanBuffer> mainGeometryBuffers_;

    VkDescriptorSet currentFrameLightingDescriptorSet_ = VK_NULL_HANDLE;
    VkDescriptorSet currentFrameGeometryDescriptorSet_ = VK_NULL_HANDLE;
    VulkanBuffer* currentFrameGeometryBuffer_ = nullptr;
};
