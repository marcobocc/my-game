#pragma once
#include <array>
#include <vector>
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/descriptors.hpp"
#include "../core/utils/structs.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/core/GameWindow.hpp"
#include "structs/assets/Material.hpp"
#include "structs/assets/Mesh.hpp"
#include "structs/components/Camera.hpp"

class VulkanGeometryPass {
public:
    static constexpr VkFormat ALBEDO_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;
    static constexpr VkFormat NORMAL_FORMAT = VK_FORMAT_R16G16B16A16_SFLOAT;
    static constexpr VkFormat DEPTH_FORMAT = VK_FORMAT_D32_SFLOAT;

    VulkanGeometryPass(const VulkanContext& context,
                       VulkanResourcesManager& resourcesManager,
                       AssetManager& assetManager,
                       GameWindow& window) :
        context_(const_cast<VulkanContext&>(context)),
        resourcesManager_(resourcesManager),
        assetManager_(assetManager),
        window_(window) {
        createUBOLayout();
    }

    ~VulkanGeometryPass() { vkDestroyDescriptorSetLayout(context_.device, perFrameUBOLayout_, nullptr); }

    std::pair<VulkanBuffer, VkDescriptorSet> createRenderContext() const {
        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        VulkanBuffer buf = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);
        VkDescriptorSet ds = createUniformBufferDescriptorSet(
                context_.device, context_.descriptorPool, perFrameUBOLayout_, buf.buffer, uboSize);
        return {std::move(buf), ds};
    }

    void destroyRenderContext(VulkanBuffer& buf) const { destroyBuffer(context_.device, buf); }

    void record(VkCommandBuffer cmd,
                VkDescriptorSet geometryDescriptorSet,
                VulkanBuffer& geometryBuffer,
                VkImageView albedoView,
                VkImageView normalView,
                VkImageView depthView,
                VkExtent2D extent,
                const Camera& camera,
                const Transform& cameraTransform,
                const std::vector<DrawCall>& drawQueue,
                const VkRect2D* viewportOverride = nullptr) {
        beginRendering(cmd, albedoView, normalView, depthView, extent, viewportOverride);
        updatePerFrameUBO(geometryBuffer, camera, cameraTransform);
        for (const auto& drawCall: drawQueue)
            renderEntity(cmd, geometryDescriptorSet, drawCall);
        vkCmdEndRendering(cmd);
    }

private:
    void createUBOLayout() {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        vkCreateDescriptorSetLayout(context_.device, &layoutInfo, nullptr, &perFrameUBOLayout_);
    }

    void updatePerFrameUBO(VulkanBuffer& buf, const Camera& camera, const Transform& cameraTransform) const {
        glm::mat4 data[2] = {cameraTransform.getViewMatrix(), camera.getProjectionMatrix()};
        updateBuffer(context_.device, buf, data, sizeof(data));
    }

    void renderEntity(VkCommandBuffer cmd, VkDescriptorSet geometryDescriptorSet, const DrawCall& drawCall) const {
        const Material* material = assetManager_.get<Material>(drawCall.renderer.materialName);
        static constexpr std::array<VkFormat, 2> colorFormats{ALBEDO_FORMAT, NORMAL_FORMAT};
        auto [pipeline, texturesDescriptorSet] = resourcesManager_.getMaterial(*material, colorFormats, DEPTH_FORMAT);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 0, 1, &geometryDescriptorSet, 0, nullptr);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout, 1, 1, &texturesDescriptorSet, 0, nullptr);

        struct PushConstants {
            glm::mat4 modelMatrix;
            glm::vec4 tint;
            glm::vec2 tiling;
            glm::vec2 offset;
            int scaleInvariantUV;
        } pc{};
        pc.modelMatrix = drawCall.transform.getModelMatrix();
        pc.tint = drawCall.renderer.tintOverride.value_or(material->getTint());
        pc.tiling = material->getTiling();
        pc.offset = material->getOffset();
        pc.scaleInvariantUV = material->getScaleInvariantUV();
        vkCmdPushConstants(cmd,
                           pipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(PushConstants),
                           &pc);

        const Mesh* mesh = assetManager_.get<Mesh>(drawCall.renderer.meshName);
        const auto& meshBuffers = resourcesManager_.getMesh(*mesh);
        std::array<VkDeviceSize, 1> offsets{};
        vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffers.vertexBuffer.buffer, offsets.data());
        vkCmdBindIndexBuffer(cmd, meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
        if (mesh->hasIndices())
            vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh->getIndices().size()), 1, 0, 0, 0);
        else
            vkCmdDraw(cmd, static_cast<uint32_t>(mesh->getVertexCount()), 1, 0, 0);
    }

    void beginRendering(VkCommandBuffer cmd,
                        VkImageView albedoView,
                        VkImageView normalView,
                        VkImageView depthView,
                        VkExtent2D extent,
                        const VkRect2D* viewportOverride = nullptr) const {
        int fbX = 0, fbY = 0, fbW = 0, fbH = 0;
        if (viewportOverride) {
            fbX = viewportOverride->offset.x;
            fbY = viewportOverride->offset.y;
            fbW = static_cast<int>(viewportOverride->extent.width);
            fbH = static_cast<int>(viewportOverride->extent.height);
        } else {
            const SceneViewport sv = window_.getSceneViewport();
            const auto [scaleX, scaleY] = window_.getContentScale();
            fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
            fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
            fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
            fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);
        }

        VkClearValue clearAlbedo{};
        clearAlbedo.color = {{0.0f, 0.0f, 0.0f, 0.0f}};
        VkClearValue clearNormal{};
        clearNormal.color = {{0.5f, 0.5f, 1.0f, 1.0f}};
        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo colorAttachments[2] = {};
        colorAttachments[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[0].imageView = albedoView;
        colorAttachments[0].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[0].clearValue = clearAlbedo;

        colorAttachments[1].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachments[1].imageView = normalView;
        colorAttachments[1].imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachments[1].clearValue = clearNormal;

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
        renderingInfo.colorAttachmentCount = 2;
        renderingInfo.pColorAttachments = colorAttachments;
        renderingInfo.pDepthAttachment = &depthAttachment;
        vkCmdBeginRendering(cmd, &renderingInfo);

        VkViewport viewport{};
        viewport.x = static_cast<float>(fbX);
        viewport.y = static_cast<float>(fbY);
        viewport.width = static_cast<float>(fbW);
        viewport.height = static_cast<float>(fbH);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{fbX, fbY}, {static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    VulkanContext& context_;
    VulkanResourcesManager& resourcesManager_;
    AssetManager& assetManager_;
    GameWindow& window_;

    VkDescriptorSetLayout perFrameUBOLayout_ = VK_NULL_HANDLE;
};
