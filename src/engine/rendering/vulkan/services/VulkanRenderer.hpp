#pragma once
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "core/objects/components/Camera.hpp"
#include "core/ui/UserInterface.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanContext.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanPipeline.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanUBO.hpp"
#include "rendering/vulkan/services/VulkanCommandManager.hpp"
#include "rendering/vulkan/services/VulkanImguiRenderer.hpp"
#include "rendering/vulkan/services/VulkanResourceCache.hpp"
#include "rendering/vulkan/services/VulkanSceneRenderer.hpp"

class VulkanRenderer {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    ~VulkanRenderer();
    VulkanRenderer(const VulkanContext& vulkanContext,
                   size_t swapchainImageCount,
                   VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                   VulkanResourceCache<VulkanPipeline>& pipelineCache,
                   VulkanSwapchain& swapchain,
                   GLFWwindow* window,
                   UserInterface* userInterface);

    bool renderFrame(size_t& currentFrame,
                     VulkanCommandManager& commandManager,
                     const VulkanSwapchain& swapchain,
                     VkQueue graphicsQueue,
                     const std::vector<DrawCall>& drawCalls,
                     const Camera& camera);

private:
    void createSynchronizationObjects();
    void waitForFrame(size_t frameIndex) const;
    bool acquireImage(size_t currentFrame, const VulkanSwapchain& swapchain, uint32_t& imageIndex) const;
    void submitAndPresent(VkCommandBuffer cmd,
                          size_t currentFrame,
                          uint32_t imageIndex,
                          VulkanCommandManager& commandManager,
                          const VulkanSwapchain& swapchain,
                          VkQueue graphicsQueue) const;
    void setImageLayoutTransition(VkCommandBuffer cmd, VkImage image, VkFormat format,
                              VkImageLayout oldLayout, VkImageLayout newLayout,
                              VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                              VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT) const;

    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
    };

    struct ImageSync {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    };

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    std::array<FrameSync, MAX_FRAMES_IN_FLIGHT> frames_{};
    std::vector<ImageSync> images_;

    VulkanUBO cameraUBO_;
    VulkanResourceCache<VulkanBuffer>& vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline>& pipelineCache_;
    VulkanSwapchain& swapchainManager_;
    VulkanSceneRenderer sceneRenderer_;
    VulkanImguiRenderer imguiRenderer_;
};
