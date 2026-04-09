#include "vulkan/services/VulkanRenderer.hpp"
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/services/VulkanSceneRenderer.hpp"

struct CameraUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

VulkanRenderer::VulkanRenderer(const VulkanContext& vulkanContext,
                               size_t swapchainImageCount,
                               VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                               VulkanResourceCache<VulkanPipeline>& pipelineCache,
                               VulkanSwapchain& swapchain) :
    device_(vulkanContext.getVkDevice()),
    physicalDevice_(vulkanContext.getVkPhysicalDevice()),
    images_(swapchainImageCount),
    cameraUBO_(vulkanContext, sizeof(CameraUBO)),
    vertexBufferCache_(vertexBufferCache),
    pipelineCache_(pipelineCache),
    swapchainManager_(swapchain),
    sceneRenderer_(vulkanContext, vertexBufferCache_, pipelineCache_, swapchainManager_, cameraUBO_) {
    createSynchronizationObjects();
}

VulkanRenderer::~VulkanRenderer() {
    vkDeviceWaitIdle(device_);
    for (auto& [inFlightFence]: frames_) {
        if (inFlightFence != VK_NULL_HANDLE) vkDestroyFence(device_, inFlightFence, nullptr);
    }
    for (auto& [imageAvailableSemaphore, renderFinishedSemaphore]: images_) {
        if (imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device_, imageAvailableSemaphore, nullptr);
        if (renderFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device_, renderFinishedSemaphore, nullptr);
    }
}

void VulkanRenderer::createSynchronizationObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& [inFlightFence]: frames_) {
        throwIfUnsuccessful(vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFence));
    }

    for (auto& [imageAvailableSemaphore, renderFinishedSemaphore]: images_) {
        throwIfUnsuccessful(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphore));
        throwIfUnsuccessful(vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphore));
    }
}

bool VulkanRenderer::renderFrame(size_t& currentFrame,
                                 VulkanCommandManager& commandManager,
                                 const VulkanSwapchain& swapchain,
                                 VkQueue graphicsQueue,
                                 const std::vector<DrawCall>& drawCalls,
                                 const CameraComponent& camera) {
    waitForFrame(currentFrame);

    CameraUBO cameraUBO{.view = camera.getViewMatrix(), .proj = camera.getProjectionMatrix()};
    cameraUBO_.updateUBO(currentFrame, &cameraUBO, sizeof(CameraUBO));

    uint32_t imageIndex = 0;
    if (!acquireImage(currentFrame, swapchain, imageIndex)) {
        return false;
    }

    commandManager.beginFrame();
    VkCommandBuffer cmd = commandManager.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(cmd);

    sceneRenderer_.recordScenePass(cmd, imageIndex, currentFrame, drawCalls);
    // TODO: Other passes go here

    submitAndPresent(cmd, currentFrame, imageIndex, commandManager, swapchain, graphicsQueue);
    commandManager.endFrame();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
}

void VulkanRenderer::waitForFrame(size_t frameIndex) const {
    VkFence fence = frames_.at(frameIndex).inFlightFence;
    vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &fence);
}

bool VulkanRenderer::acquireImage(size_t currentFrame, const VulkanSwapchain& swapchain, uint32_t& imageIndex) const {
    size_t acquireIndex = currentFrame % images_.size();
    if (!swapchain.acquireNextImage(images_.at(acquireIndex).imageAvailableSemaphore, imageIndex)) {
        return false;
    }
    if (imageIndex >= swapchain.getImageCount()) {
        return false;
    }
    return true;
}

void VulkanRenderer::submitAndPresent(VkCommandBuffer cmd,
                                      size_t currentFrame,
                                      uint32_t imageIndex,
                                      VulkanCommandManager& commandManager,
                                      const VulkanSwapchain& swapchain,
                                      VkQueue graphicsQueue) const {
    VulkanCommandManager::endCommandBuffer(cmd);
    size_t acquireIndex = currentFrame % images_.size();
    VkFence fence = frames_.at(currentFrame).inFlightFence;
    commandManager.submitCommandBuffer(cmd,
                                       images_.at(acquireIndex).imageAvailableSemaphore,
                                       images_.at(imageIndex).renderFinishedSemaphore,
                                       fence);

    swapchain.present(graphicsQueue, images_.at(imageIndex).renderFinishedSemaphore, imageIndex);
}
