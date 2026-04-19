#include "VulkanRenderingOrchestrator.hpp"
#include <volk.h>
#include "VulkanCommandManager.hpp"
#include "VulkanSwapchainManager.hpp"
#include "renderers/VulkanImguiRenderer.hpp"
#include "renderers/VulkanSceneRenderer.hpp"
#include "rendering/vulkan/core/error_handling.hpp"

VulkanRenderingOrchestrator::VulkanRenderingOrchestrator(VulkanContext& context,
                                                         VulkanSceneRenderer& sceneRenderer,
                                                         VulkanImguiRenderer& imguiRenderer,
                                                         VulkanCommandManager& commandManager,
                                                         VulkanSwapchainManager& swapchainManager) :
    context_(context),
    sceneRenderer_(sceneRenderer),
    imguiRenderer_(imguiRenderer),
    commandManager_(commandManager),
    swapchainManager_(swapchainManager) {
    initFrameSync();
}

VulkanRenderingOrchestrator::~VulkanRenderingOrchestrator() {
    vkDeviceWaitIdle(context_.device);
    destroyFrameSync();
}

void VulkanRenderingOrchestrator::enqueueForDrawing(const Mesh& mesh,
                                                    const Material& material,
                                                    const Transform& transform) const {
    sceneRenderer_.enqueueForDrawing(mesh, material, transform);
}

bool VulkanRenderingOrchestrator::renderFrame(const Camera& camera) {
    waitForCurrentFrame();
    uint32_t imageIndex = 0;
    if (!acquireImage(imageIndex)) return false;
    VkCommandBuffer cmd = beginFrame();
    recordCommands(cmd, imageIndex, camera);
    endFrame(cmd, imageIndex);
    advanceFrame();
    return true;
}

// ------------------- Frame sync -------------------

void VulkanRenderingOrchestrator::initFrameSync() {
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

void VulkanRenderingOrchestrator::destroyFrameSync() {
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

void VulkanRenderingOrchestrator::waitForCurrentFrame() {
    VkFence fence = currentFrame().inFlightFence;
    if (fence != VK_NULL_HANDLE) {
        vkWaitForFences(context_.device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(context_.device, 1, &fence);
    }
}

void VulkanRenderingOrchestrator::advanceFrame() { currentFrameIndex_ = (currentFrameIndex_ + 1) % frames_.size(); }

VulkanRenderingOrchestrator::FrameSync& VulkanRenderingOrchestrator::currentFrame() {
    return frames_.at(currentFrameIndex_);
}

// ------------------- Frame stages -------------------

bool VulkanRenderingOrchestrator::acquireImage(uint32_t& imageIndex) {
    if (!swapchainManager_.acquireNextImage(context_, currentFrame().imageAvailable, imageIndex)) return false;
    if (imageIndex >= swapchainManager_.swapchain().swapchainImages.size()) return false;
    return true;
}

VkCommandBuffer VulkanRenderingOrchestrator::beginFrame() const {
    commandManager_.beginFrame();
    VkCommandBuffer cmd = commandManager_.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(cmd);
    return cmd;
}

void VulkanRenderingOrchestrator::prepareSceneCanvas(VkCommandBuffer cmd, uint32_t imageIndex) const {
    const VulkanSwapchain& swapchain = swapchainManager_.swapchain();

    VkClearValue clearColor{};
    clearColor.color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    VkClearValue clearDepth{};
    clearDepth.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchain.swapchainImageViews[imageIndex];
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColor;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = swapchain.depthBuffer.view;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue = clearDepth;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, swapchain.swapchainExtent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(swapchain.swapchainExtent.height);
    viewport.width = static_cast<float>(swapchain.swapchainExtent.width);
    viewport.height = -static_cast<float>(swapchain.swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{{0, 0}, swapchain.swapchainExtent};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void VulkanRenderingOrchestrator::recordCommands(VkCommandBuffer cmd, uint32_t imageIndex, const Camera& camera) {
    transitionToRenderLayouts(cmd, imageIndex);
    prepareSceneCanvas(cmd, imageIndex);
    sceneRenderer_.drawScene(cmd, camera);
    vkCmdEndRendering(cmd);
    imguiRenderer_.recordUIPass(cmd, imageIndex);
}

void VulkanRenderingOrchestrator::endFrame(VkCommandBuffer cmd, uint32_t imageIndex) {
    transitionToPresent(cmd, imageIndex);
    VulkanCommandManager::endCommandBuffer(cmd);
    submit(cmd, imageIndex);
    commandManager_.endFrame();
}

void VulkanRenderingOrchestrator::submit(VkCommandBuffer cmd, uint32_t imageIndex) {
    const auto& frame = currentFrame();
    commandManager_.submitCommandBuffer(cmd, frame.imageAvailable, frame.renderFinished, frame.inFlightFence);
    swapchainManager_.present(context_.graphicsQueue, frame.renderFinished, imageIndex);
}

// ------------------- Transitions -------------------

void VulkanRenderingOrchestrator::transitionToRenderLayouts(VkCommandBuffer cmd, uint32_t imageIndex) const {
    VkImage color = swapchainManager_.swapchain().swapchainImages[imageIndex];
    VkImage depth = swapchainManager_.swapchain().depthBuffer.image;
    transitionColorAttachment(cmd, color);
    transitionDepthAttachment(cmd, depth);
}

void VulkanRenderingOrchestrator::transitionToPresent(VkCommandBuffer cmd, uint32_t imageIndex) const {
    VkImage image = swapchainManager_.swapchain().swapchainImages[imageIndex];
    transitionToPresent(cmd, image);
}

void VulkanRenderingOrchestrator::transitionColorAttachment(VkCommandBuffer cmd, VkImage image) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
}

void VulkanRenderingOrchestrator::transitionDepthAttachment(VkCommandBuffer cmd, VkImage image) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
}

void VulkanRenderingOrchestrator::transitionToPresent(VkCommandBuffer cmd, VkImage image) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);
}
