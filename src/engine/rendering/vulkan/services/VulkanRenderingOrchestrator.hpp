#pragma once
#include <glm/vec3.hpp>
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
                                VulkanScenePass& scenePass,
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

    void enqueueForDrawing(const Renderer&, const Transform&, std::string objectId) const;
    void enqueueForOutline(const Renderer&, const Transform&, std::string objectId) const;
    void submitGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const;
    bool renderFrame(const Camera& camera, const Transform& cameraTransform);

private:
    bool acquireImage(uint32_t& imageIndex);
    VkCommandBuffer beginFrame() const;
    void recordCommands(VkCommandBuffer, uint32_t imageIndex, const Camera& camera, const Transform& cameraTransform);
    void endFrame(VkCommandBuffer, uint32_t imageIndex);
    void submit(VkCommandBuffer, uint32_t imageIndex);
    void transitionToPresent(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void transitionObjectIdToShaderRead(VkCommandBuffer cmd) const;

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
    VulkanGizmoPass& gizmoPass_;
    VulkanObjectIdPass& objectIdPass_;
    VulkanPickingBackend& pickingBackend_;
    VulkanOutlinePass& outlinePass_;
    VulkanUIPass& uiPass_;
    VulkanCommandManager& commandManager_;
    VulkanSwapchainManager& swapchainManager_;
    RendererSettings& settings_;
};
