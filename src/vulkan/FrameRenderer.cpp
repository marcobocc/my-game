#include "vulkan/FrameRenderer.hpp"
#include <volk.h>
#include "vulkan/VulkanBuffer.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

FrameRenderer::FrameRenderer(VulkanPipelinesManager& pipelinesManager,
                             VulkanVertexBuffersManager& vertexBuffersManager,
                             VulkanSwapchainManager& swapchainManager) :
    pipelinesManager_(pipelinesManager),
    vertexBuffersManager_(vertexBuffersManager),
    swapchainManager_(swapchainManager) {}

void FrameRenderer::renderFrame(VkCommandBuffer commandBuffer,
                                uint32_t imageIndex,
                                const std::vector<DrawCall>& drawCalls,
                                VkDescriptorSet cameraDescriptorSet) const {
    beginRenderPass(commandBuffer, imageIndex);
    setupViewportAndScissor(commandBuffer);

    for (const auto& drawCall: drawCalls) {
        renderEntity(commandBuffer, drawCall, cameraDescriptorSet);
    }

    endRenderPass(commandBuffer);
}

void FrameRenderer::beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex) const {
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
}

void FrameRenderer::setupViewportAndScissor(VkCommandBuffer cmd) const {
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
}

void FrameRenderer::renderEntity(VkCommandBuffer cmd,
                                 const DrawCall& drawCall,
                                 VkDescriptorSet cameraDescriptorSet) const {
    const auto& mesh = *drawCall.mesh;
    const auto& [name, vertexShaderPath, fragmentShaderPath] = *drawCall.material;
    const auto& modelMatrix = drawCall.modelMatrix;

    VulkanBuffer* vertexBuffer = vertexBuffersManager_.createOrGetVertexBuffer(
            mesh.name, mesh.vertices.data(), mesh.vertices.size() * sizeof(float));

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

    auto* pipeline = pipelinesManager_.createOrGetPipeline(name, vertexInputInfo, vertexShaderPath, fragmentShaderPath);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getVkPipeline());
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

    } else {
        vkCmdDraw(cmd, static_cast<uint32_t>(mesh.getVertexCount()), 1, 0, 0);
    }
}

void FrameRenderer::endRenderPass(VkCommandBuffer cmd) { vkCmdEndRenderPass(cmd); }
