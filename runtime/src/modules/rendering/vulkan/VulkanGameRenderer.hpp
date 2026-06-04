#pragma once
#include <memory>
#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include "../../../structs/RendererSettings.hpp"
#include "../GameRenderData.hpp"
#include "core/RenderGraph.hpp"
#include "core/utils/structs.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanGeometryPass;
class VulkanLightingPass;
class VulkanShadowPass;
class VulkanSkyPass;
class VulkanSwapchainManager;
class VulkanFrameManager;
class VulkanRenderTargetManager;
class VulkanResourcesManager;
class AssetLoader;

class VulkanGameRenderer {
public:
    VulkanGameRenderer(VulkanContext& context,
                       VulkanFrameManager& frameManager,
                       VulkanRenderTargetManager& renderTargetManager,
                       VulkanGeometryPass& geometryPass,
                       VulkanLightingPass& lightingPass,
                       VulkanSwapchainManager& swapchainManager,
                       RendererSettings& settings,
                       VulkanResourcesManager& resourcesManager,
                       AssetLoader& assetLoader);

    ~VulkanGameRenderer();

    // Render one frame for the given game render data.
    bool renderFrame(const GameRenderData& renderData);

    RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height);
    void destroyRenderTarget(RenderTargetHandle handle);

private:
    // --- Frame helpers ---
    void recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const GameRenderData& renderData);
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
    std::unique_ptr<VulkanShadowPass> shadowPass_;
    std::unique_ptr<VulkanSkyPass> skyPass_;

    // --- Render graph ---
    std::optional<VulkanRenderGraph<GameRenderData>> graph_;
    ResourceHandle colorTargetHandle_;
    ResourceHandle gbufferAlbedoHandle_;
    ResourceHandle gbufferNormalHandle_;
    ResourceHandle gbufferDepthHandle_;
    ResourceHandle shadowDepthHandle_;

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
