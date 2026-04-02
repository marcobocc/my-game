#include "vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>

VulkanGraphicsBackend::~VulkanGraphicsBackend() {
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        if (imageAvailableSemaphores_.at(i) != VK_NULL_HANDLE)
            vkDestroySemaphore(device_.getVkDevice(), imageAvailableSemaphores_.at(i), nullptr);
        if (renderFinishedSemaphores_.at(i) != VK_NULL_HANDLE)
            vkDestroySemaphore(device_.getVkDevice(), renderFinishedSemaphores_.at(i), nullptr);
        if (inFlightFences_.at(i) != VK_NULL_HANDLE)
            vkDestroyFence(device_.getVkDevice(), inFlightFences_.at(i), nullptr);
    }
}

VulkanGraphicsBackend::VulkanGraphicsBackend(GLFWwindow* window) :
    window_(window),
    debugMessenger_(instance_.getVkInstance()),
    device_(instance_.getVkInstance()),
    commandManager_(device_.getVkDevice(),
                    device_.getGraphicsQueueFamilyIndex(),
                    device_.getVkGraphicsQueue(),
                    MAX_FRAMES_IN_FLIGHT),
    swapchainManager_(window_, instance_.getVkInstance(), device_.getVkPhysicalDevice(), device_.getVkDevice()),
    pipelinesManager_(device_.getVkDevice(), swapchainManager_.renderPass()),
    vertexBuffersManager_(device_.getVkDevice(), device_.getVkPhysicalDevice()) {

    if (!window) throw std::runtime_error("Window pointer is null");
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        throwIfUnsuccessful(
                vkCreateSemaphore(device_.getVkDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_.at(i)));
        throwIfUnsuccessful(
                vkCreateSemaphore(device_.getVkDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_.at(i)));
        throwIfUnsuccessful(vkCreateFence(device_.getVkDevice(), &fenceInfo, nullptr, &inFlightFences_.at(i)));
    }
}

void VulkanGraphicsBackend::draw(const MeshComponent& mesh,
                                 const MaterialComponent& material,
                                 const glm::mat4& modelMatrix) {
    drawQueue_.push_back({&mesh, &material, modelMatrix});
}

void VulkanGraphicsBackend::renderFrame() {
    VkSemaphore imageAvailableSemaphore = imageAvailableSemaphores_.at(currentFrame_);
    VkSemaphore renderFinishedSemaphore = renderFinishedSemaphores_.at(currentFrame_);
    VkFence inFlightFence = inFlightFences_.at(currentFrame_);
    VkResult fenceStatus = vkGetFenceStatus(device_.getVkDevice(), inFlightFence);
    if (fenceStatus == VK_NOT_READY) {
        // Previous frame is still in flight, skip rendering this frame to avoid blocking UI
        return;
    }
    vkResetFences(device_.getVkDevice(), 1, &inFlightFence);
    uint32_t imageIndex = 0;
    if (!swapchainManager_.acquireNextImage(imageAvailableSemaphore, imageIndex)) return;
    if (imageIndex >= swapchainManager_.imageCount()) return;
    commandManager_.beginFrame();
    VkCommandBuffer cmd = commandManager_.allocateCommandBuffer();
    VulkanCommandManager::beginCommandBuffer(cmd);
    VkClearValue clearColor = {{{0.1f, 0.1f, 0.1f, 1.0f}}};
    VkRenderPassBeginInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpInfo.renderPass = swapchainManager_.renderPass();
    rpInfo.framebuffer = swapchainManager_.framebuffer(imageIndex);
    rpInfo.renderArea.offset = {0, 0};
    rpInfo.renderArea.extent = swapchainManager_.extent();
    rpInfo.clearValueCount = 1;
    rpInfo.pClearValues = &clearColor;
    vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Set dynamic viewport and scissor
    VkExtent2D extent = swapchainManager_.extent();
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (auto& [mesh, material, modelMatrix]: drawQueue_) {
        renderEntity(cmd, *mesh, *material, modelMatrix);
    }

    vkCmdEndRenderPass(cmd);
    VulkanCommandManager::endCommandBuffer(cmd);
    commandManager_.submitCommandBuffer(cmd, imageAvailableSemaphore, renderFinishedSemaphore);
    swapchainManager_.present(device_.getVkGraphicsQueue(), renderFinishedSemaphore, imageIndex);
    commandManager_.endFrame();
    drawQueue_.clear();
    currentFrame_ = (currentFrame_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

inline void VulkanGraphicsBackend::renderEntity(VkCommandBuffer cmd,
                                                const MeshComponent& mesh,
                                                const MaterialComponent& material,
                                                const glm::mat4& modelMatrix) {
    // Create or fetch vertex buffer
    VulkanBuffer* vertexBuffer = vertexBuffersManager_.createOrGetVertexBuffer(
            mesh.name, mesh.vertices.data(), mesh.vertices.size() * sizeof(float));

    // Build Vulkan vertex attributes from MeshComponent
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

    // Vertex input binding
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

    // Get or create pipeline
    auto* pipeline = pipelinesManager_.createOrGetPipeline(
            material.name, vertexInputInfo, material.vertexShaderPath, material.fragmentShaderPath);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getVkPipeline());

    // Bind vertex buffer
    VkBuffer buf = vertexBuffer->getVkBuffer();
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &buf, offsets);

    // Push model matrix
    vkCmdPushConstants(
            cmd, pipeline->getVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMatrix);

    // Draw
    if (mesh.hasIndices()) {
        // TODO: implement index buffer binding
        // vkCmdBindIndexBuffer(...)
        // vkCmdDrawIndexed(...)
    } else {
        vkCmdDraw(cmd, static_cast<uint32_t>(mesh.getVertexCount()), 1, 0, 0);
    }
}
