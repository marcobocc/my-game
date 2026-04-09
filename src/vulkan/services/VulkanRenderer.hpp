#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "core/Material.hpp"
#include "core/Mesh.hpp"
#include "core/components/CameraComponent.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/raii_wrappers/VulkanPipeline.hpp"
#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "vulkan/raii_wrappers/VulkanUBO.hpp"
#include "vulkan/services/VulkanCommandManager.hpp"
#include "vulkan/services/VulkanResourceCache.hpp"

struct DrawCall {
    const Mesh* mesh;
    const Material* material;
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
                   VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                   VulkanResourceCache<VulkanPipeline>& pipelineCache,
                   VkRenderPass renderPass,
                   VulkanSwapchain& swapchain);

    bool renderFrame(size_t& currentFrame,
                     VulkanCommandManager& commandManager,
                     const VulkanSwapchain& swapchain,
                     VkQueue graphicsQueue,
                     const std::vector<DrawCall>& drawCalls,
                     const CameraComponent& camera);

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
                             size_t currentFrame,
                             const std::vector<DrawCall>& drawCalls);
    void beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void setupViewportAndScissor(VkCommandBuffer cmd) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, size_t currentFrame);
    static void endRenderPass(VkCommandBuffer cmd);

    struct FrameSync {
        VkFence inFlightFence = VK_NULL_HANDLE;
    };

    struct ImageSync {
        VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
        VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    };

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkRenderPass renderPass_;
    std::array<FrameSync, MAX_FRAMES_IN_FLIGHT> frames_{};
    std::vector<ImageSync> images_;

    VulkanUBO cameraUBO_;
    VulkanResourceCache<VulkanBuffer>& vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline>& pipelineCache_;
    VulkanSwapchain& swapchainManager_;
};
