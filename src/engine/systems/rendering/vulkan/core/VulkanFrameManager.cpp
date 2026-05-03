#include "VulkanFrameManager.hpp"
#include <volk.h>
#include "VulkanCommandManager.hpp"
#include "VulkanSwapchainManager.hpp"
#include "utils/error_handling.hpp"
#include "utils/structs.hpp"

VulkanFrameManager::VulkanFrameManager(VulkanContext& context,
                                       VulkanCommandManager& commandManager,
                                       VulkanSwapchainManager& swapchainManager) :
    context_(context),
    commandManager_(commandManager),
    swapchainManager_(swapchainManager) {
    initFrameSync();
}

VulkanFrameManager::~VulkanFrameManager() { destroyFrameSync(); }

bool VulkanFrameManager::acquireImage(uint32_t& imageIndex) {
    if (!swapchainManager_.acquireNextImage(context_, currentFrame().imageAvailable, imageIndex)) return false;
    if (imageIndex >= swapchainManager_.swapchain().swapchainImages.size()) return false;
    return true;
}

VkCommandBuffer VulkanFrameManager::beginFrame() {
    commandManager_.beginFrame();
    currentCommandBuffer_ = commandManager_.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(currentCommandBuffer_);
    return currentCommandBuffer_;
}

void VulkanFrameManager::endFrame(VkCommandBuffer cmd) {
    VulkanCommandManager::endCommandBuffer(cmd);
    commandManager_.endFrame();
}

void VulkanFrameManager::submit(uint32_t imageIndex) {
    const auto& frame = currentFrame();
    commandManager_.submitCommandBuffer(
            currentCommandBuffer_, frame.imageAvailable, frame.renderFinished, frame.inFlightFence);
    swapchainManager_.present(context_.graphicsQueue, frame.renderFinished, imageIndex);
}

void VulkanFrameManager::waitForCurrentFrame() {
    VkFence fence = currentFrame().inFlightFence;
    if (fence != VK_NULL_HANDLE) {
        vkWaitForFences(context_.device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context_.device, 1, &fence);
    }
}

void VulkanFrameManager::advanceFrame() { currentFrameIndex_ = (currentFrameIndex_ + 1) % frames_.size(); }

VulkanFrameManager::FrameSync& VulkanFrameManager::currentFrame() { return frames_.at(currentFrameIndex_); }

void VulkanFrameManager::initFrameSync() {
    const size_t frameCount = swapchainManager_.swapchain().swapchainImages.size();
    frames_.resize(frameCount);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame: frames_) {
        throwIfUnsuccessful(vkCreateFence(context_.device, &fenceInfo, nullptr, &frame.inFlightFence));
        throwIfUnsuccessful(vkCreateSemaphore(context_.device, &semaphoreInfo, nullptr, &frame.imageAvailable));
        throwIfUnsuccessful(vkCreateSemaphore(context_.device, &semaphoreInfo, nullptr, &frame.renderFinished));
    }
}

void VulkanFrameManager::destroyFrameSync() {
    for (auto& frame: frames_) {
        if (frame.inFlightFence != VK_NULL_HANDLE) {
            vkWaitForFences(context_.device, 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(context_.device, frame.inFlightFence, nullptr);
            frame.inFlightFence = VK_NULL_HANDLE;
        }
        if (frame.imageAvailable != VK_NULL_HANDLE) {
            vkDestroySemaphore(context_.device, frame.imageAvailable, nullptr);
            frame.imageAvailable = VK_NULL_HANDLE;
        }
        if (frame.renderFinished != VK_NULL_HANDLE) {
            vkDestroySemaphore(context_.device, frame.renderFinished, nullptr);
            frame.renderFinished = VK_NULL_HANDLE;
        }
    }
}
