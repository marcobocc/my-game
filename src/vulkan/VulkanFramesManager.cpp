#include "vulkan/VulkanFramesManager.hpp"
#include <volk.h>
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanErrorHandling.hpp"

VulkanFramesManager::VulkanFramesManager(VkDevice device,
                                         VkPhysicalDevice physicalDevice,
                                         size_t swapchainImageCount,
                                         VulkanPipelinesManager& pipelinesManager,
                                         VulkanVertexBuffersManager& vertexBuffersManager,
                                         VulkanSwapchainManager& swapchainManager) :
    device_(device),
    images_(swapchainImageCount),
    frameRenderer_(pipelinesManager, vertexBuffersManager, swapchainManager) {
    createSynchronizationObjects();
}

VulkanFramesManager::~VulkanFramesManager() {
    for (auto& [inFlightFence]: frames_) {
        if (inFlightFence != VK_NULL_HANDLE) vkDestroyFence(device_, inFlightFence, nullptr);
    }
    for (auto& [imageAvailableSemaphore, renderFinishedSemaphore]: images_) {
        if (imageAvailableSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device_, imageAvailableSemaphore, nullptr);
        if (renderFinishedSemaphore != VK_NULL_HANDLE) vkDestroySemaphore(device_, renderFinishedSemaphore, nullptr);
    }
}

void VulkanFramesManager::createSynchronizationObjects() {
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

bool VulkanFramesManager::renderFrame(size_t& currentFrame,
                                      VulkanCommandManager& commandManager,
                                      const VulkanSwapchainManager& swapchainManager,
                                      VkQueue graphicsQueue,
                                      const std::vector<DrawCall>& drawCalls,
                                      VkDescriptorSet cameraDescriptorSet) const {
    waitForFrame(currentFrame);

    uint32_t imageIndex = 0;
    if (!acquireImage(currentFrame, swapchainManager, imageIndex)) {
        return false;
    }

    commandManager.beginFrame();
    VkCommandBuffer cmd = commandManager.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(cmd);
    frameRenderer_.renderFrame(cmd, imageIndex, drawCalls, cameraDescriptorSet);
    submitAndPresent(cmd, currentFrame, imageIndex, commandManager, swapchainManager, graphicsQueue);
    commandManager.endFrame();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return true;
}

void VulkanFramesManager::waitForFrame(size_t frameIndex) const {
    VkFence fence = frames_.at(frameIndex).inFlightFence;
    vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &fence);
}

bool VulkanFramesManager::acquireImage(size_t currentFrame,
                                       const VulkanSwapchainManager& swapchainManager,
                                       uint32_t& imageIndex) const {
    size_t acquireIndex = currentFrame % images_.size();
    if (!swapchainManager.acquireNextImage(images_.at(acquireIndex).imageAvailableSemaphore, imageIndex)) {
        return false;
    }
    if (imageIndex >= swapchainManager.imageCount()) {
        return false;
    }
    return true;
}

void VulkanFramesManager::submitAndPresent(VkCommandBuffer cmd,
                                           size_t currentFrame,
                                           uint32_t imageIndex,
                                           VulkanCommandManager& commandManager,
                                           const VulkanSwapchainManager& swapchainManager,
                                           VkQueue graphicsQueue) const {
    VulkanCommandManager::endCommandBuffer(cmd);
    size_t acquireIndex = currentFrame % images_.size();
    VkFence fence = frames_.at(currentFrame).inFlightFence;
    commandManager.submitCommandBuffer(cmd,
                                       images_.at(acquireIndex).imageAvailableSemaphore,
                                       images_.at(imageIndex).renderFinishedSemaphore,
                                       fence);

    swapchainManager.present(graphicsQueue, images_.at(imageIndex).renderFinishedSemaphore, imageIndex);
}
