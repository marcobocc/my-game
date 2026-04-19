#include "rendering/vulkan/services/VulkanSceneRenderer.hpp"
#include <glm/glm.hpp>
#include <volk.h>
#include "assets/types/mesh/MeshData.hpp"
#include "assets/types/shader/Shader.hpp"
#include "assets/types/texture/Texture.hpp"

VulkanSceneRenderer::VulkanSceneRenderer(const VulkanContext& vulkanContext,
                                         VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                                         VulkanResourceCache<VulkanPipeline>& pipelineCache,
                                         VulkanResourceCache<VulkanTexture>& textureCache,
                                         VulkanSwapchain& swapchain,
                                         VulkanUBO& cameraUBO,
                                         VulkanTextureSet* textureSet,
                                         AssetManager& assetManager) :
    device_(vulkanContext.getVkDevice()),
    physicalDevice_(vulkanContext.getVkPhysicalDevice()),
    vertexBufferCache_(vertexBufferCache),
    pipelineCache_(pipelineCache),
    textureCache_(textureCache),
    swapchain_(swapchain),
    cameraUBO_(cameraUBO),
    assetManager_(assetManager),
    textureSet_(textureSet) {}

void VulkanSceneRenderer::recordScenePass(VkCommandBuffer cmd,
                                          uint32_t imageIndex,
                                          size_t currentFrame,
                                          const std::vector<DrawCall>& drawCalls) {
    beginRendering(cmd, imageIndex);
    setupViewportAndScissor(cmd);
    for (const auto& drawCall: drawCalls) {
        renderEntity(cmd, drawCall, currentFrame);
    }
    endRendering(cmd);
}

void VulkanSceneRenderer::beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const {
    VkClearValue clearColor{};
    clearColor.color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    VkClearValue clearDepth{};
    clearDepth.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = swapchain_.getVkImageView(imageIndex);
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColor;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = swapchain_.getDepthImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.clearValue = clearDepth;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = swapchain_.getVkExtent();
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;
    renderingInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(cmd, &renderingInfo);
}

void VulkanSceneRenderer::endRendering(VkCommandBuffer cmd) const { vkCmdEndRendering(cmd); }

void VulkanSceneRenderer::setupViewportAndScissor(VkCommandBuffer cmd) const {
    VkExtent2D extent = swapchain_.getVkExtent();
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = static_cast<float>(extent.height);
    viewport.width = static_cast<float>(extent.width);
    viewport.height = -static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void VulkanSceneRenderer::renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, size_t currentFrame) {
    struct PushConstants {
        glm::mat4 modelMatrix;
        glm::vec4 baseColor;
    };

    // Fetch assets using AssetManager
    const auto& mesh = drawCall.mesh;
    const auto& material = drawCall.material;
    const auto& modelMatrix = drawCall.transform.getModelMatrix();

    const MeshData* meshAsset = assetManager_.get<MeshData>(mesh.name);
    const Shader* shaderAsset = assetManager_.get<Shader>(material.shaderName);
    const Texture* texture = assetManager_.get<Texture>(material.textureName);

    VulkanBuffer* vertexBuffer =
            vertexBufferCache_.createOrGet(meshAsset->getName(),
                                           device_,
                                           physicalDevice_,
                                           meshAsset->getVertices().size() * sizeof(float),
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                           meshAsset->getVertices().data());

    VulkanBuffer* indexBuffer = nullptr;
    if (meshAsset->hasIndices()) {
        indexBuffer = vertexBufferCache_.createOrGet(meshAsset->getName() + "_indices",
                                                     device_,
                                                     physicalDevice_,
                                                     meshAsset->getIndices().size() * sizeof(uint32_t),
                                                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                     meshAsset->getIndices().data());
    }

    std::vector<VkVertexInputAttributeDescription> vkAttributes;
    uint32_t location = 0;
    for (const auto& [offset, componentCount]: meshAsset->getVertexAttributes()) {
        VkFormat format = VK_FORMAT_UNDEFINED;
        if (componentCount == 2)
            format = VK_FORMAT_R32G32_SFLOAT;
        else if (componentCount == 3)
            format = VK_FORMAT_R32G32B32_SFLOAT;
        else if (componentCount == 4)
            format = VK_FORMAT_R32G32B32A32_SFLOAT;

        VkVertexInputAttributeDescription desc{};
        desc.location = location++;
        desc.binding = 0;
        desc.format = format;
        desc.offset = static_cast<uint32_t>(offset * sizeof(float));
        vkAttributes.push_back(desc);
    }

    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(float) * meshAsset->getVertexStride();
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vkAttributes.size());
    vertexInputInfo.pVertexAttributeDescriptions = vkAttributes.data();

    constexpr uint32_t pushConstantSize = sizeof(PushConstants);

    auto* pipeline = pipelineCache_.createOrGet(shaderAsset->getName(),
                                                device_,
                                                shaderAsset->getVertexBytecode(),
                                                shaderAsset->getFragmentBytecode(),
                                                vertexInputInfo,
                                                pushConstantSize,
                                                cameraUBO_.getDescriptorSetLayout(),
                                                textureSet_->getLayout(),
                                                swapchain_.getSwapchainImageFormat(),
                                                swapchain_.getDepthFormat());

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getVkPipeline());

    VkDescriptorSet cameraDescriptorSet = cameraUBO_.getDescriptorSet(currentFrame);

    VkDescriptorSet textureSet = VK_NULL_HANDLE;
    if (!material.textureName.empty() && texture) {
        auto it = textureDescriptorSets_.find(material.textureName);
        if (it == textureDescriptorSets_.end()) {
            VulkanTexture* tex = textureCache_.createOrGet(
                    material.textureName,
                    device_,
                    physicalDevice_,
                    static_cast<uint32_t>(texture->getWidth()),
                    static_cast<uint32_t>(texture->getHeight()),
                    std::vector<unsigned char>(texture->getImageData(),
                                               texture->getImageData() +
                                                       texture->getWidth() * texture->getHeight() * 4));
            if (tex && tex->imageView != VK_NULL_HANDLE && tex->sampler != VK_NULL_HANDLE) {
                if (textureSet_ != nullptr) {
                    textureSet = textureSet_->allocateSet();
                    textureSet_->updateSet(textureSet, tex->imageView, tex->sampler);
                    textureDescriptorSets_[material.textureName] = textureSet;
                } else {
                    fprintf(stderr,
                            "[Vulkan] Error: textureSet_ is null. Cannot allocate texture set for '%s'.\n",
                            material.textureName.c_str());
                }
            }
        } else {
            textureSet = it->second;
        }
    }
    if (textureSet == VK_NULL_HANDLE) {
        fprintf(stderr,
                "[Vulkan] Warning: No valid texture descriptor set available for object '%s'. Skipping draw.\n",
                drawCall.mesh.name.c_str());
        return;
    }
    VkDescriptorSet sets[2] = {cameraDescriptorSet, textureSet};
    vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->getVkPipelineLayout(), 0, 2, sets, 0, nullptr);

    VkBuffer buf = vertexBuffer->getVkBuffer();
    std::array<VkDeviceSize, 1> offsets = {};
    vkCmdBindVertexBuffers(cmd, 0, 1, &buf, offsets.data());

    PushConstants pushConstants{.modelMatrix = modelMatrix, .baseColor = material.baseColor};

    vkCmdPushConstants(
            cmd, pipeline->getVkPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &pushConstants);

    if (meshAsset->hasIndices() && indexBuffer != nullptr) {
        VkBuffer idxBuf = indexBuffer->getVkBuffer();
        vkCmdBindIndexBuffer(cmd, idxBuf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(meshAsset->getIndices().size()), 1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd, static_cast<uint32_t>(meshAsset->getVertexCount()), 1, 0, 0);
    }
}
