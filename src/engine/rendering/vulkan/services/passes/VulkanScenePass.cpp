#include "VulkanScenePass.hpp"
#include <volk.h>
#include "../../../../assets/types/Material.hpp"
#include "../../../../assets/types/Mesh.hpp"
#include "rendering/vulkan/core/buffers.hpp"
#include "rendering/vulkan/core/descriptors.hpp"
#include "rendering/vulkan/core/structs.hpp"

VulkanScenePass::VulkanScenePass(const VulkanContext& context,
                                 VulkanResourcesManager& resourcesManager,
                                 AssetManager& assetManager,
                                 GameWindow& window) :
    assetManager_(assetManager),
    resourcesManager_(resourcesManager),
    context_(const_cast<VulkanContext&>(context)),
    window_(window) {
    createPerFrameUBO();
}

VulkanScenePass::~VulkanScenePass() {
    destroyBuffer(context_.device, perFrameUBO_.buffer);
    vkDestroyDescriptorSetLayout(context_.device, perFrameUBOLayout_, nullptr);
}

void VulkanScenePass::createPerFrameUBO() {
    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding = 0;
    cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBinding.descriptorCount = 1;
    cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &cameraBinding;
    vkCreateDescriptorSetLayout(context_.device, &layoutInfo, nullptr, &perFrameUBOLayout_);

    constexpr std::size_t uboSize = sizeof(glm::mat4) * 2;
    perFrameUBO_.buffer = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);
    perFrameUBO_.descriptorSet = createUniformBufferDescriptorSet(
            context_.device, context_.descriptorPool, perFrameUBOLayout_, perFrameUBO_.buffer.buffer, uboSize);
}

void VulkanScenePass::updatePerFrameUBO(const Camera& camera, const Transform& cameraTransform) const {
    glm::mat4 data[2] = {cameraTransform.getViewMatrix(), camera.getProjectionMatrix()};
    updateBuffer(context_.device, perFrameUBO_.buffer, data, sizeof(data));
}

void VulkanScenePass::enqueueForDrawing(const Renderer& renderer, const Transform& transform, std::string objectId) {
    drawQueue_.push_back({renderer, transform, std::move(objectId)});
}

void VulkanScenePass::record(VkCommandBuffer cmd,
                             VkImageView colorView,
                             VkImageView depthView,
                             VkExtent2D extent,
                             const Camera& camera,
                             const Transform& cameraTransform) {
    beginRendering(cmd, colorView, depthView, extent);

    updatePerFrameUBO(camera, cameraTransform);
    for (const auto& drawCall: drawQueue_)
        renderEntity(cmd, drawCall);
    drawQueue_.clear();

    vkCmdEndRendering(cmd);
}

void VulkanScenePass::renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const {
    const Material* material = assetManager_.get<Material>(drawCall.renderer.materialName);
    auto [pipeline, texturesDescriptorSet] = resourcesManager_.getMaterial(*material);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &perFrameUBO_.descriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 1, 1, &texturesDescriptorSet, 0, nullptr);

    struct PushConstants {
        glm::mat4 modelMatrix;
        glm::vec4 baseColor;
    } pushConstants;
    pushConstants.modelMatrix = drawCall.transform.getModelMatrix();
    pushConstants.baseColor = drawCall.renderer.baseColorOverride.value_or(material->getBaseColor());

    vkCmdPushConstants(cmd,
                       pipeline->layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushConstants),
                       &pushConstants);

    const Mesh* mesh = assetManager_.get<Mesh>(drawCall.renderer.meshName);
    const auto& meshBuffers = resourcesManager_.getMesh(*mesh);
    std::array<VkDeviceSize, 1> offsets = {};
    vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffers.vertexBuffer.buffer, offsets.data());
    vkCmdBindIndexBuffer(cmd, meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    if (mesh->hasIndices()) {
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh->getIndices().size()), 1, 0, 0, 0);
        return;
    }
    vkCmdDraw(cmd, static_cast<uint32_t>(mesh->getVertexCount()), 1, 0, 0);
}

void VulkanScenePass::beginRendering(VkCommandBuffer cmd,
                                     VkImageView colorView,
                                     VkImageView depthView,
                                     VkExtent2D extent) const {
    const SceneViewport sv = window_.getSceneViewport();
    const auto [scaleX, scaleY] = window_.getContentScale();
    const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
    const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
    const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
    const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

    VkClearValue clearColor{};
    clearColor.color = {{0.1f, 0.1f, 0.1f, 1.0f}};
    VkClearValue clearDepth{};
    clearDepth.depthStencil = {1.0f, 0};

    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = colorView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue = clearColor;

    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = depthView;
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue = clearDepth;

    VkRenderingInfo renderingInfo{};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea = {{0, 0}, extent};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = &depthAttachment;

    vkCmdBeginRendering(cmd, &renderingInfo);

    VkViewport viewport{};
    viewport.x = static_cast<float>(fbX);
    viewport.y = static_cast<float>(fbY + fbH);
    viewport.width = static_cast<float>(fbW);
    viewport.height = -static_cast<float>(fbH);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{{fbX, fbY}, {static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH)}};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}
