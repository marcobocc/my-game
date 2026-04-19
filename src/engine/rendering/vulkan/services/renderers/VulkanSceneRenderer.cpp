#include "VulkanSceneRenderer.hpp"
#include <volk.h>
#include "assets/types/mesh/MeshData.hpp"
#include "rendering/vulkan/core/buffers.hpp"
#include "rendering/vulkan/core/descriptors.hpp"
#include "rendering/vulkan/core/structs.hpp"

VulkanSceneRenderer::VulkanSceneRenderer(const VulkanContext& vulkanContext,
                                         VulkanResourcesManager& resourcesManager,
                                         AssetManager& assetManager) :
    assetManager_(assetManager),
    resourcesManager_(resourcesManager),
    vulkanContext_(const_cast<VulkanContext&>(vulkanContext)) {
    createPerFrameUBO();
}

VulkanSceneRenderer::~VulkanSceneRenderer() {
    destroyBuffer(vulkanContext_.device, perFrameUBO_.buffer);
    vkDestroyDescriptorSetLayout(vulkanContext_.device, perFrameUBOLayout_, nullptr);
}

void VulkanSceneRenderer::createPerFrameUBO() {
    // All shaders must define per frame uniform buffer at set 0. The bindings for this set must be:
    // - Binding 0 = Camera view and projection matrices

    VkDescriptorSetLayoutBinding cameraBinding{};
    cameraBinding.binding = 0;
    cameraBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    cameraBinding.descriptorCount = 1;
    cameraBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &cameraBinding;

    vkCreateDescriptorSetLayout(vulkanContext_.device, &layoutInfo, nullptr, &perFrameUBOLayout_);

    constexpr std::size_t uboSize = sizeof(glm::mat4) * 2;
    perFrameUBO_.buffer = createUniformBuffer(vulkanContext_.device, vulkanContext_.physicalDevice, uboSize);
    perFrameUBO_.descriptorSet = createUniformBufferDescriptorSet(vulkanContext_.device,
                                                                  vulkanContext_.descriptorPool,
                                                                  perFrameUBOLayout_,
                                                                  perFrameUBO_.buffer.buffer,
                                                                  uboSize);
}

void VulkanSceneRenderer::updatePerFrameUBO(const Camera& camera) const {
    glm::mat4 data[2] = {camera.getViewMatrix(), camera.getProjectionMatrix()};
    updateBuffer(vulkanContext_.device, perFrameUBO_.buffer, data, sizeof(data));
}

void VulkanSceneRenderer::enqueueForDrawing(const Mesh& mesh, const Material& material, const Transform& transform) {
    drawQueue_.push_back({mesh, material, transform});
}

void VulkanSceneRenderer::drawScene(VkCommandBuffer cmd, const Camera& camera) {
    updatePerFrameUBO(camera);
    for (const auto& drawCall: drawQueue_)
        renderEntity(cmd, drawCall);
    drawQueue_.clear();
}

void VulkanSceneRenderer::renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const {
    auto [pipeline, materialDescriptorSet] = resourcesManager_.getMaterial(drawCall.material);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &perFrameUBO_.descriptorSet, 0, nullptr);
    vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 1, 1, &materialDescriptorSet, 0, nullptr);

    PushConstants pushConstants{
            .modelMatrix = drawCall.transform.getModelMatrix(),
            .baseColor = drawCall.material.baseColor,
    };
    vkCmdPushConstants(cmd,
                       pipeline->layout,
                       VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                       0,
                       sizeof(PushConstants),
                       &pushConstants);

    const MeshData* meshData = assetManager_.get<MeshData>(drawCall.mesh.name);
    const auto& meshBuffers = resourcesManager_.getMesh(*meshData);
    std::array<VkDeviceSize, 1> offsets = {};
    vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffers.vertexBuffer.buffer, offsets.data());
    vkCmdBindIndexBuffer(cmd, meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    if (meshData->hasIndices()) {
        vkCmdDrawIndexed(cmd, static_cast<uint32_t>(meshData->getIndices().size()), 1, 0, 0, 0);
        return;
    }
    vkCmdDraw(cmd, static_cast<uint32_t>(meshData->getVertexCount()), 1, 0, 0);
}
