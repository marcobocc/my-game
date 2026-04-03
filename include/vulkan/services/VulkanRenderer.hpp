#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "vulkan/resource_managers/VulkanPipelinesManager.hpp"
#include "vulkan/resource_managers/VulkanVertexBuffersManager.hpp"
#include "vulkan/services/VulkanCommandManager.hpp"

struct DrawCall {
    const MeshComponent* mesh;
    const MaterialComponent* material;
    glm::mat4 modelMatrix;
};

class VulkanRenderer {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    ~VulkanRenderer();
    VulkanRenderer(VkDevice device,
                   VkPhysicalDevice physicalDevice,
                   size_t swapchainImageCount,
                   VulkanPipelinesManager& pipelinesManager,
                   VulkanVertexBuffersManager& vertexBuffersManager,
                   VulkanSwapchain& swapchain);

    bool renderFrame(size_t& currentFrame,
                     VulkanCommandManager& commandManager,
                     const VulkanSwapchain& swapchain,
                     VkQueue graphicsQueue,
                     const std::vector<DrawCall>& drawCalls,
                     VkDescriptorSet cameraDescriptorSet) const;

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
    void recordCommandBuffer(VkCommandBuffer cmd,
                             uint32_t imageIndex,
                             const std::vector<DrawCall>& drawCalls,
                             VkDescriptorSet cameraDescriptorSet) const;
    void beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void setupViewportAndScissor(VkCommandBuffer cmd) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, VkDescriptorSet cameraDescriptorSet) const;
    static void endRenderPass(VkCommandBuffer cmd);

    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
    };

    struct ImageSync {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    };

    VkDevice device_;
    std::array<FrameSync, MAX_FRAMES_IN_FLIGHT> frames_{};
    std::vector<ImageSync> images_;

    VulkanPipelinesManager& pipelinesManager_;
    VulkanVertexBuffersManager& vertexBuffersManager_;
    VulkanSwapchain& swapchainManager_;
};
