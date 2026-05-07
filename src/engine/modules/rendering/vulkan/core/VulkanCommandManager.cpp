#include "VulkanCommandManager.hpp"
#include <array>
#include <volk.h>
#include "utils/error_handling.hpp"

VulkanCommandManager::VulkanCommandManager(const VulkanContext& vulkanContext,
                                           const VulkanSwapchainManager& swapchainManager) :
    device_(vulkanContext.device),
    graphicsQueueFamilyIndex_(vulkanContext.graphicsQueueFamilyIndex),
    graphicsQueue_(vulkanContext.graphicsQueue),
    frames_(swapchainManager.swapchain().swapchainImages.size()),
    maxFramesInFlight_(swapchainManager.swapchain().swapchainImages.size()) {
    createCommandPools();
    createFences();
}

VulkanCommandManager::~VulkanCommandManager() {
    for (auto& [commandPool, frameFence]: frames_) {
        if (frameFence != VK_NULL_HANDLE) {
            vkWaitForFences(device_, 1, &frameFence, VK_TRUE, UINT64_MAX);
            vkDestroyFence(device_, frameFence, nullptr);
        }
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device_, commandPool, nullptr);
        }
    }
}

void VulkanCommandManager::beginFrame() {
    auto& [commandPool, frameFence] = frames_.at(currentFrame_);
    vkResetCommandPool(device_, commandPool, 0);
}

void VulkanCommandManager::endFrame() { currentFrame_ = (currentFrame_ + 1) % maxFramesInFlight_; }

VkCommandBuffer VulkanCommandManager::allocateCommandBuffer(VkCommandBufferLevel level) {
    auto& [commandPool, frameFence] = frames_.at(currentFrame_);
    VkCommandBufferAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc.commandPool = commandPool;
    alloc.level = level;
    alloc.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    throwIfUnsuccessful(vkAllocateCommandBuffers(device_, &alloc, &cmd));
    return cmd;
}

void VulkanCommandManager::submitCommandBuffer(VkCommandBuffer cmd,
                                               VkSemaphore waitSemaphore,
                                               VkSemaphore signalSemaphore,
                                               VkFence fence) {
    auto& [commandPool, frameFence] = frames_.at(currentFrame_);
    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::array<VkPipelineStageFlags, 1> waitStages = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    if (waitSemaphore != VK_NULL_HANDLE) {
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &waitSemaphore;
        submit.pWaitDstStageMask = waitStages.data();
    }

    if (signalSemaphore != VK_NULL_HANDLE) {
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &signalSemaphore;
    }

    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    VkFence fenceToUse = fence != VK_NULL_HANDLE ? fence : frameFence;
    throwIfUnsuccessful(vkQueueSubmit(graphicsQueue_, 1, &submit, fenceToUse));
}

void VulkanCommandManager::beginCommandBuffer(VkCommandBuffer cmd, VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin.flags = flags;
    throwIfUnsuccessful(vkBeginCommandBuffer(cmd, &begin));
}

void VulkanCommandManager::endCommandBuffer(VkCommandBuffer cmd) { throwIfUnsuccessful(vkEndCommandBuffer(cmd)); }

void VulkanCommandManager::createCommandPools() {
    for (auto& [commandPool, frameFence]: frames_) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex_;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        throwIfUnsuccessful(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool));
    }
}

void VulkanCommandManager::createFences() {
    for (auto& [commandPool, frameFence]: frames_) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        throwIfUnsuccessful(vkCreateFence(device_, &fenceInfo, nullptr, &frameFence));
    }
}
