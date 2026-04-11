#include "vulkan/services/VulkanRenderer.hpp"
#include <GLFW/glfw3.h>
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "vulkan/services/VulkanImguiRenderer.hpp"
#include "vulkan/services/VulkanSceneRenderer.hpp"

struct CameraUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

VulkanRenderer::VulkanRenderer(const VulkanContext& vulkanContext,
                               size_t swapchainImageCount,
                               VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                               VulkanResourceCache<VulkanPipeline>& pipelineCache,
                               VulkanSwapchain& swapchain,
                               GLFWwindow* window,
                               UserInterface* userInterface) :
    device_(vulkanContext.getVkDevice()),
    physicalDevice_(vulkanContext.getVkPhysicalDevice()),
    images_(swapchainImageCount),
    cameraUBO_(vulkanContext, sizeof(CameraUBO)),
    vertexBufferCache_(vertexBufferCache),
    pipelineCache_(pipelineCache),
    swapchainManager_(swapchain),
    sceneRenderer_(vulkanContext, vertexBufferCache_, pipelineCache_, swapchainManager_, cameraUBO_),
    imguiRenderer_(vulkanContext, swapchain, swapchainImageCount, window, userInterface) {
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

void VulkanRenderer::setImageLayoutTransition(VkCommandBuffer cmd,
                                              VkImage image,
                                              VkFormat format,
                                              VkImageLayout oldLayout,
                                              VkImageLayout newLayout,
                                              VkPipelineStageFlags srcStage,
                                              VkPipelineStageFlags dstStage,
                                              VkImageAspectFlags aspectMask) const {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrier.dstAccessMask = 0;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
               newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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
    if (imageIndex >= swapchain.getImageCount()) {
        return false;
    }

    commandManager.beginFrame();
    VkCommandBuffer cmd = commandManager.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(cmd);

    VkImage swapchainImage = swapchain.getVkImage(imageIndex);
    this->setImageLayoutTransition(cmd,
                                   swapchainImage,
                                   swapchain.getSwapchainImageFormat(),
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
    VkImage depthImage = swapchain.getDepthImage();
    this->setImageLayoutTransition(cmd,
                                   depthImage,
                                   swapchain.getDepthFormat(),
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                   VK_IMAGE_ASPECT_DEPTH_BIT);

    sceneRenderer_.recordScenePass(cmd, imageIndex, currentFrame, drawCalls);
    imguiRenderer_.recordUIPass(cmd, imageIndex);

    submitAndPresent(cmd, currentFrame, imageIndex, commandManager, swapchain, graphicsQueue);
    commandManager.endFrame();
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}

void VulkanRenderer::waitForFrame(size_t frameIndex) const {
    VkFence fence = frames_.at(frameIndex).inFlightFence;
    if (fence != VK_NULL_HANDLE) {
        vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
        vkResetFences(device_, 1, &fence);
    }
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
    // --- Insert image layout transition before presenting ---
    VkImage swapchainImage = swapchain.getVkImage(imageIndex);
    this->setImageLayoutTransition(cmd,
                                   swapchainImage,
                                   swapchain.getSwapchainImageFormat(),
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                   VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT);
    VulkanCommandManager::endCommandBuffer(cmd);
    size_t acquireIndex = currentFrame % images_.size();
    VkFence fence = frames_.at(currentFrame).inFlightFence;
    if (fence != VK_NULL_HANDLE) {
        commandManager.submitCommandBuffer(cmd,
                                           images_.at(acquireIndex).imageAvailableSemaphore,
                                           images_.at(imageIndex).renderFinishedSemaphore,
                                           fence);
    }
    swapchain.present(graphicsQueue, images_.at(imageIndex).renderFinishedSemaphore, imageIndex);
}
