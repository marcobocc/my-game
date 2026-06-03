#pragma once
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include "EditorRenderData.hpp"
#include "modules/rendering/vulkan/core/RenderGraph.hpp"
#include "modules/rendering/vulkan/core/utils/structs.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"
#include "structs/RendererSettings.hpp"

class GameWindow;
class VulkanGeometryPass;
class VulkanLightingPass;
class VulkanGridPass;
class VulkanGizmoPass;
class VulkanObjectIdPass;
class VulkanOutlinePass;
class VulkanUIPass;
class VulkanSwapchainManager;
class VulkanFrameManager;
class VulkanRenderTargetManager;

class VulkanBackend {
public:
    VulkanBackend(GameWindow& window,
                  VulkanContext& context,
                  VulkanFrameManager& frameManager,
                  VulkanRenderTargetManager& renderTargetManager,
                  VulkanGeometryPass& geometryPass,
                  VulkanLightingPass& lightingPass,
                  VulkanGridPass& gridPass,
                  VulkanGizmoPass& gizmoPass,
                  VulkanObjectIdPass& objectIdPass,
                  VulkanOutlinePass& outlinePass,
                  VulkanUIPass& uiPass,
                  VulkanSwapchainManager& swapchainManager,
                  RendererSettings& settings);

    ~VulkanBackend();

    void setDrawCallback(std::function<void()> callback);

    // Render one frame for the given renderData view.
    // Off-screen cameras (camera.renderTarget.isValid()) are queued and flushed
    // at the start of the next swapchain frame.
    bool renderFrame(const EditorRenderData& renderData);

    // Render only the UI pass — skips all geometry, lighting, and gizmo passes.
    bool renderUIOnly();

    // Queue an off-screen camera render to be processed in the next swapchain frame.
    // The camera will render using the scene's draw queue from the next renderFrame() call.
    void queueOffscreenCamera(const Camera& camera, const Transform& cameraTransform) {
        if (camera.renderTarget.isValid()) {
            pendingOffscreenCameras_.push_back({camera, cameraTransform});
        }
    }

    RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height);
    void destroyRenderTarget(RenderTargetHandle handle);
    VkDescriptorSet getRenderTargetImGuiId(RenderTargetHandle handle) const;
    VulkanRenderTarget getRenderTarget(RenderTargetHandle handle) const;

    const std::vector<std::string>& objectIdMap() const { return objectIdMap_; }

private:
    // --- Frame helpers ---
    void recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const EditorRenderData& renderData);
    void executeRenderGraph(VkCommandBuffer cmd,
                            VkImage colorImage,
                            VkImageView colorView,
                            VkExtent2D extent,
                            const EditorRenderData& renderData);


    // --- Render graph setup ---
    void setupRenderGraph(VkFormat colorFormat, VkImageUsageFlags colorUsage, VkExtent2D extent);

    // --- Dependencies ---
    GameWindow& window_;
    VulkanContext& context_;
    RendererSettings& settings_;
    VulkanSwapchainManager& swapchainManager_;
    VulkanFrameManager& frameManager_;
    VulkanRenderTargetManager& renderTargetManager_;
    VulkanGeometryPass& geometryPass_;
    VulkanLightingPass& lightingPass_;
    VulkanGridPass& gridPass_;
    VulkanGizmoPass& gizmoPass_;
    VulkanObjectIdPass& objectIdPass_;
    VulkanOutlinePass& outlinePass_;
    VulkanUIPass& uiPass_;

    // --- Render graph ---
    std::optional<VulkanRenderGraph<EditorRenderData>> renderGraph_;
    ResourceHandle colorTargetHandle_;
    ResourceHandle gbufferAlbedoHandle_;
    ResourceHandle gbufferNormalHandle_;
    ResourceHandle gbufferDepthHandle_;
    ResourceHandle objectIdColorHandle_;

    std::vector<std::string> objectIdMap_;

    // --- Frame state ---
    uint32_t frameCount_ = 0;
    VkFormat swapchainColorFormat_ = VK_FORMAT_UNDEFINED;

    // Per-frame resources for the main swapchain render (one slot per swapchain image).
    std::vector<VkDescriptorSet> mainLightingDescriptorSets_;
    std::vector<VkDescriptorSet> mainOutlineDescriptorSets_;
    std::vector<VkDescriptorSet> mainGeometryDescriptorSets_;
    std::vector<VulkanBuffer> mainGeometryBuffers_;

    // Per-frame descriptor sets and buffers for each off-screen render target (by handle index).
    std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> renderTargetLightingDescriptorSets_;
    std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> renderTargetOutlineDescriptorSets_;
    std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> renderTargetGeometryDescriptorSets_;
    std::unordered_map<uint32_t, std::vector<VulkanBuffer>> renderTargetGeometryBuffers_;

    // ImGui descriptor sets for rendering targets in the editor UI (managed by handle index).
    std::unordered_map<uint32_t, VkDescriptorSet> renderTargetImGuiSets_;

    // Current frame descriptor sets (updated before each render graph execution)
    VkDescriptorSet currentFrameLightingDescriptorSet_ = VK_NULL_HANDLE;
    VkDescriptorSet currentFrameOutlineDescriptorSet_ = VK_NULL_HANDLE;
    VkDescriptorSet currentFrameGeometryDescriptorSet_ = VK_NULL_HANDLE;
    VulkanBuffer* currentFrameGeometryBuffer_ = nullptr;

    struct PendingOffscreenCamera {
        Camera camera;
        Transform cameraTransform;
    };
    std::vector<PendingOffscreenCamera> pendingOffscreenCameras_;
};
