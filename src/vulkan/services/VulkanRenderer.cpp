#include "vulkan/services/VulkanRenderer.hpp"
#include <volk.h>
#include "vulkan/VulkanErrorHandling.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"

struct CameraUBO {
    glm::mat4 view;
    glm::mat4 proj;
};

VulkanRenderer::VulkanRenderer(VkDevice device,
                               VkPhysicalDevice physicalDevice,
                               size_t swapchainImageCount,
                               VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                               VulkanResourceCache<VulkanPipeline>& pipelineCache,
                               VkRenderPass renderPass,
                               VulkanSwapchain& swapchain) :
    device_(device),
    physicalDevice_(physicalDevice),
    renderPass_(renderPass),
    images_(swapchainImageCount),
    cameraUBO_(device, physicalDevice, sizeof(CameraUBO)),
    vertexBufferCache_(vertexBufferCache),
    pipelineCache_(pipelineCache),
    swapchainManager_(swapchain) {
    createSynchronizationObjects();
}

VulkanRenderer::~VulkanRenderer() {
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
    recordCommandBuffer(cmd, imageIndex, currentFrame, drawCalls);
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

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer cmd,
                                         uint32_t imageIndex,
                                         size_t currentFrame,
                                         const std::vector<DrawCall>& drawCalls) {
    beginRenderPass(cmd, imageIndex);
    setupViewportAndScissor(cmd);

    for (const auto& drawCall: drawCalls) {
        renderEntity(cmd, drawCall, currentFrame);
    }

    endRenderPass(cmd);
}

void VulkanRenderer::beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex) const {
    VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = swapchainManager_.getVkRenderPass();
    rpInfo.framebuffer = swapchainManager_.getVkFramebuffer(imageIndex);
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = swapchainManager_.getVkExtent();
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderer::setupViewportAndScissor(VkCommandBuffer cmd) const {
    VkExtent2D extent = swapchainManager_.getVkExtent();
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(extent.height);
    viewport.width = static_cast<float>(extent.width);
    // Use negative viewport with VK_KHR_maintenance1 to flip Y-axis
    viewport.height = -static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void VulkanRenderer::renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, size_t currentFrame) {
    const auto& mesh = *drawCall.mesh;
    const auto& [name, vertexShaderPath, fragmentShaderPath] = *drawCall.material;
    const auto& modelMatrix = drawCall.modelMatrix;

    VulkanBuffer* vertexBuffer =
            vertexBufferCache_.createOrGet(mesh.name,
                                           device_,
                                           physicalDevice_,
                                           mesh.vertices.size() * sizeof(float),
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           mesh.vertices.data());

    VulkanBuffer* indexBuffer = nullptr;
    if (mesh.hasIndices()) {
        indexBuffer = vertexBufferCache_.createOrGet(mesh.name + "_indices",
                                                     device_,
                                                     physicalDevice_,
                                                     mesh.indices.size() * sizeof(uint32_t),
                                                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                     mesh.indices.data());
    }

    std::vector<VkVertexInputAttributeDescription> vkAttributes;
    uint32_t location = 0;
    for (const auto& attr: mesh.attributes) {
        VkFormat format = VK_FORMAT_UNDEFINED;
        if (attr.componentCount == 2)
            format = VK_FORMAT_R32G32_SFLOAT;
        else if (attr.componentCount == 3)
            format = VK_FORMAT_R32G32B32_SFLOAT;
        else if (attr.componentCount == 4)
            format = VK_FORMAT_R32G32B32A32_SFLOAT;

        VkVertexInputAttributeDescription desc{};
        desc.location = location++;
        desc.binding = 0;
        desc.format = format;
        desc.offset = static_cast<uint32_t>(attr.offset * sizeof(float));
        vkAttributes.push_back(desc);
    }

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(float) * mesh.vertexStride;
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vkAttributes.data();

    auto* pipeline = pipelineCache_.createOrGet(name,
                                                device_,
                                                renderPass_,
                                                vertexShaderPath,
                                                fragmentShaderPath,
                                                vertexInputInfo,
                                                cameraUBO_.getDescriptorSetLayout());

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getVkPipeline());

    VkDescriptorSet cameraDescriptorSet = cameraUBO_.getDescriptorSet(currentFrame);

    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipeline->getVkPipelineLayout(),
                            0,
                            1,
                            &cameraDescriptorSet,
                            0,
                            nullptr);

    VkBuffer buf = vertexBuffer->getVkBuffer();
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &buf, offsets);

    vkCmdPushConstants(
            cmd, pipeline->getVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

    if (mesh.hasIndices()) {
        VkBuffer idxBuf = indexBuffer->getVkBuffer();
        vkCmdBindIndexBuffer(cmd, idxBuf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd, static_cast<uint32_t>(mesh.getVertexCount()), 1, 0, 0);
    }
}

void VulkanRenderer::endRenderPass(VkCommandBuffer cmd) { vkCmdEndRenderPass(cmd); }
