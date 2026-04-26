#include "VulkanRenderingOrchestrator.hpp"
#include <volk.h>
#include "VulkanCommandManager.hpp"
#include "VulkanSwapchainManager.hpp"
#include "core/GameWindow.hpp"
#include "passes/VulkanGridPass.hpp"
#include "passes/VulkanObjectIdPass.hpp"
#include "passes/VulkanOutlinePass.hpp"
#include "passes/VulkanScenePass.hpp"
#include "passes/VulkanUIPass.hpp"
#include "rendering/vulkan/core/error_handling.hpp"

VulkanRenderingOrchestrator::VulkanRenderingOrchestrator(GameWindow& window,
                                                         VulkanContext& context,
                                                         VulkanScenePass& scenePass,
                                                         VulkanGridPass& gridPass,
                                                         VulkanObjectIdPass& objectIdPass,
                                                         VulkanOutlinePass& outlinePass,
                                                         VulkanUIPass& uiPass,
                                                         VulkanCommandManager& commandManager,
                                                         VulkanSwapchainManager& swapchainManager,
                                                         RendererSettings& settings) :
    window_(window),
    context_(context),
    scenePass_(scenePass),
    gridPass_(gridPass),
    objectIdPass_(objectIdPass),
    outlinePass_(outlinePass),
    uiPass_(uiPass),
    commandManager_(commandManager),
    swapchainManager_(swapchainManager),
    settings_(settings) {
    initFrameSync();
    window_.onWindowResize([this](int, int, int, int) {
        swapchainManager_.recreate(context_);
        const auto& extent = swapchainManager_.swapchain().swapchainExtent;
        objectIdPass_.resize(extent.width, extent.height);
        outlinePass_.resize(extent.width, extent.height);
    });
}

VulkanRenderingOrchestrator::~VulkanRenderingOrchestrator() {
    vkDeviceWaitIdle(context_.device);
    destroyFrameSync();
}

void VulkanRenderingOrchestrator::enqueueForDrawing(const Renderer& renderer,
                                                    const Transform& transform,
                                                    std::string objectId) const {
    scenePass_.enqueueForDrawing(renderer, transform, std::move(objectId));
}

void VulkanRenderingOrchestrator::enqueueForOutline(const Renderer& renderer,
                                                    const Transform& transform,
                                                    std::string objectId) const {
    outlinePass_.enqueueForOutline(renderer, transform, std::move(objectId));
}

bool VulkanRenderingOrchestrator::renderFrame(const Camera& camera, const Transform& cameraTransform) {
    waitForCurrentFrame();
    objectIdPass_.resolvePickResult();

    uint32_t imageIndex = 0;
    if (!acquireImage(imageIndex)) return false;
    VkCommandBuffer cmd = beginFrame();
    recordCommands(cmd, imageIndex, camera, cameraTransform);
    endFrame(cmd, imageIndex);
    advanceFrame();
    return true;
}

void VulkanRenderingOrchestrator::requestPick(uint32_t x, uint32_t y) { objectIdPass_.requestPick(x, y); }

std::optional<std::string> VulkanRenderingOrchestrator::getPickResult() { return objectIdPass_.getPickResult(); }

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

void VulkanRenderingOrchestrator::recordCommands(VkCommandBuffer cmd,
                                                 uint32_t imageIndex,
                                                 const Camera& camera,
                                                 const Transform& cameraTransform) {
    objectIdPass_.record(cmd, scenePass_.getDrawQueue(), camera, cameraTransform);
    scenePass_.record(cmd, imageIndex, camera, cameraTransform);
    outlinePass_.record(cmd,
                        imageIndex,
                        swapchainManager_.swapchain(),
                        objectIdPass_.objectIdBufferImageView(),
                        objectIdPass_.objectIdBufferSampler());
    if (settings_.enableGrid) gridPass_.record(cmd, imageIndex, camera, cameraTransform);
    uiPass_.record(cmd, imageIndex);
}

void VulkanRenderingOrchestrator::endFrame(VkCommandBuffer cmd, uint32_t imageIndex) {
    transitionToPresent(cmd, imageIndex);
    VulkanCommandManager::endCommandBuffer(cmd);
    submit(cmd, imageIndex);
    commandManager_.endFrame();
}

void VulkanRenderingOrchestrator::transitionToPresent(VkCommandBuffer cmd, uint32_t imageIndex) const {
    VkImage image = swapchainManager_.swapchain().swapchainImages[imageIndex];
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = 0;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
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

void VulkanRenderingOrchestrator::submit(VkCommandBuffer cmd, uint32_t imageIndex) {
    const auto& frame = currentFrame();
    commandManager_.submitCommandBuffer(cmd, frame.imageAvailable, frame.renderFinished, frame.inFlightFence);
    swapchainManager_.present(context_.graphicsQueue, frame.renderFinished, imageIndex);
}
