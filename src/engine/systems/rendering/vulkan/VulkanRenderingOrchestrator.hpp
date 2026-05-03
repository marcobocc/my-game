#pragma once
#include <glm/vec3.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "RenderTarget.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"
#include "data/settings/RendererSettings.hpp"
#include "rendergraph/RenderGraph.hpp"
#include "utils/structs.hpp"

class GameWindow;
class VulkanCommandManager;
class VulkanGeometryPass;
class VulkanLightingPass;
class VulkanGridPass;
class VulkanGizmoPass;
class VulkanObjectIdPass;
class VulkanPickingBackend;
class VulkanOutlinePass;
class VulkanUIPass;
class VulkanSwapchainManager;

class VulkanRenderingOrchestrator {
public:
    VulkanRenderingOrchestrator(GameWindow& window,
                                VulkanContext& context,
                                VulkanGeometryPass& geometryPass,
                                VulkanLightingPass& lightingPass,
                                VulkanGridPass& gridPass,
                                VulkanGizmoPass& gizmoPass,
                                VulkanObjectIdPass& objectIdPass,
                                VulkanPickingBackend& pickingBackend,
                                VulkanOutlinePass& outlinePass,
                                VulkanUIPass& uiPass,
                                VulkanCommandManager& commandManager,
                                VulkanSwapchainManager& swapchainManager,
                                RendererSettings& settings);

    ~VulkanRenderingOrchestrator();

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId);
    void enqueueForOutline(const Renderer&, const Transform&, std::string objectId) const;
    void submitGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const;
    bool renderFrame(const Camera& camera, const Transform& cameraTransform);

    RenderTargetHandle createRenderTarget(uint32_t width, uint32_t height);
    void destroyRenderTarget(RenderTargetHandle handle);
    VkDescriptorSet getRenderTargetImGuiId(RenderTargetHandle handle) const;
    void renderToTarget(RenderTargetHandle handle,
                        const Camera& camera,
                        const Transform& cameraTransform,
                        const std::vector<DrawCall>& drawQueue);
    void renderToTarget(RenderTargetHandle handle, const Camera& camera, const Transform& cameraTransform);

private:
    void setupGraph();
    bool acquireImage(uint32_t& imageIndex);
    VkCommandBuffer beginFrame() const;
    void recordCommands(VkCommandBuffer, uint32_t imageIndex, const Camera& camera, const Transform& cameraTransform);
    void endFrame(VkCommandBuffer, uint32_t imageIndex);
    void submit(VkCommandBuffer, uint32_t imageIndex);

    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
    };
    std::vector<FrameSync> frames_;
    size_t currentFrameIndex_ = 0;
    void initFrameSync();
    void destroyFrameSync();
    void waitForCurrentFrame();
    void advanceFrame();
    FrameSync& currentFrame();

    const Camera* currentCamera_ = nullptr;
    const Transform* currentCameraTransform_ = nullptr;
    uint32_t currentImageIndex_ = 0;

    GameWindow& window_;
    VulkanContext& context_;
    VulkanGeometryPass& geometryPass_;
    VulkanLightingPass& lightingPass_;
    VulkanGridPass& gridPass_;
    VulkanGizmoPass& gizmoPass_;
    VulkanObjectIdPass& objectIdPass_;
    VulkanPickingBackend& pickingBackend_;
    VulkanOutlinePass& outlinePass_;
    VulkanUIPass& uiPass_;
    VulkanCommandManager& commandManager_;
    VulkanSwapchainManager& swapchainManager_;
    RendererSettings& settings_;

    std::vector<DrawCall> drawQueue_;

    RenderGraph graph_;

    ResourceHandle swapchainColorHandle_;
    ResourceHandle gbufferAlbedoHandle_;
    ResourceHandle gbufferNormalHandle_;
    ResourceHandle gbufferDepthHandle_;
    ResourceHandle objectIdColorHandle_;
    ResourceHandle objectIdDepthHandle_;

    // --- Render targets ---
    std::vector<std::unique_ptr<RenderTargetData>> renderTargets_;

    struct PendingRenderTargetJob {
        RenderTargetHandle handle;
        Camera camera;
        Transform cameraTransform;
        std::vector<DrawCall> drawQueue;
    };
    std::vector<PendingRenderTargetJob> pendingRenderTargetJobs_;

    void allocateRenderTargetImages(RenderTargetData& rt);
    void freeRenderTargetImages(RenderTargetData& rt);
    void executePendingRenderTargetJobs(VkCommandBuffer cmd);
    void renderCameraToTarget(VkCommandBuffer cmd,
                              RenderTargetData& rt,
                              const Camera& camera,
                              const Transform& cameraTransform,
                              const std::vector<DrawCall>& drawQueue);
};
