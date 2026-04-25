#pragma once
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>
#include "../core/structs.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/RendererSettings.hpp"

class GameWindow;
struct VulkanContext;
class VulkanCommandManager;
class VulkanScenePass;
class VulkanGridPass;
class VulkanPickingPass;
class VulkanUIPass;
class VulkanSwapchainManager;

class VulkanRenderingOrchestrator {
public:
    VulkanRenderingOrchestrator(GameWindow& window,
                                VulkanContext& context,
                                VulkanScenePass& scenePass,
                                VulkanGridPass& gridPass,
                                VulkanPickingPass& pickingPass,
                                VulkanUIPass& uiPass,
                                VulkanCommandManager& commandManager,
                                VulkanSwapchainManager& swapchainManager,
                                RendererSettings& settings);

    ~VulkanRenderingOrchestrator();

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId) const;
    bool renderFrame(const Camera& camera, const Transform& cameraTransform);

    void requestPick(uint32_t x, uint32_t y);
    std::optional<std::string> getPickResult();

private:
    bool acquireImage(uint32_t& imageIndex);
    VkCommandBuffer beginFrame() const;
    void recordCommands(VkCommandBuffer, uint32_t imageIndex, const Camera& camera, const Transform& cameraTransform);
    void endFrame(VkCommandBuffer, uint32_t imageIndex);
    void submit(VkCommandBuffer, uint32_t imageIndex);
    void transitionToPresent(VkCommandBuffer cmd, uint32_t imageIndex) const;

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

    GameWindow& window_;
    VulkanContext& context_;
    VulkanScenePass& scenePass_;
    VulkanGridPass& gridPass_;
    VulkanPickingPass& pickingPass_;
    VulkanUIPass& uiPass_;
    VulkanCommandManager& commandManager_;
    VulkanSwapchainManager& swapchainManager_;
    RendererSettings& settings_;
};
