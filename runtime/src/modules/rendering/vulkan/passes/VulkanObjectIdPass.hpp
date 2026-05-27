#pragma once
#include <string>
#include <vector>
#include <volk.h>
#include "../../../../modules/asset_management/asset_types/Mesh.hpp"
#include "../../../asset_management/asset_types/Shader.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/structs.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanObjectIdPass {
public:
    VulkanObjectIdPass(const VulkanContext& context,
                       AssetLoader& assetLoader,
                       VulkanResourcesManager& resourcesManager) :
        context_(context),
        assetLoader_(assetLoader),
        resourcesManager_(resourcesManager) {
        createUBO();
        initPipeline();
    }

    ~VulkanObjectIdPass() { destroyBuffer(context_.device, perFrameUBOBuffer_); }

    std::vector<std::string> record(VkCommandBuffer cmd,
                                    const std::vector<DrawCall>& outlineQueue,
                                    const Camera& camera,
                                    const Transform& cameraTransform,
                                    const GameWindow& window,
                                    VkImageView colorImageView,
                                    uint32_t imageWidth,
                                    uint32_t imageHeight) {
        if (pipeline_ == nullptr || outlineQueue.empty()) return {};

        glm::mat4 uboData[2] = {cameraTransform.getViewMatrix(), camera.getProjectionMatrix()};
        updateBuffer(context_.device, perFrameUBOBuffer_, uboData, sizeof(uboData));

        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

        VkClearValue clearColor{};
        clearColor.color.uint32[0] = 0xFFFFFFFF;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorImageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearColor;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, {imageWidth, imageHeight}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->layout, 0, 1, &perFrameUBODescriptorSet_, 0, nullptr);

        std::vector<std::string> objectIdMap;
        objectIdMap.reserve(outlineQueue.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(outlineQueue.size()); ++i) {
            const DrawCall& dc = outlineQueue[i];
            objectIdMap.push_back(dc.objectId);

            struct ObjectIdPush {
                glm::mat4 model;
                uint32_t objectId;
            } push{};
            push.model = dc.transform.getModelMatrix();
            push.objectId = i + 1;
            vkCmdPushConstants(cmd,
                               pipeline_->layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(push),
                               &push);

            const Mesh* mesh = assetLoader_.get<Mesh>(dc.renderer.meshName);
            const auto& meshBuffers = resourcesManager_.getMesh(*mesh);
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(cmd, 0, 1, &meshBuffers.vertexBuffer.buffer, &offset);
            vkCmdBindIndexBuffer(cmd, meshBuffers.indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
            if (mesh->hasIndices())
                vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh->getIndices().size()), 1, 0, 0, 0);
            else
                vkCmdDraw(cmd, static_cast<uint32_t>(mesh->getVertexCount()), 1, 0, 0);
        }

        vkCmdEndRendering(cmd);
        return objectIdMap;
    }

private:
    void createUBO() {
        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        perFrameUBOBuffer_ = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);
    }

    void initPipeline() {
        const Shader* shader = assetLoader_.get<Shader>(OBJECT_ID_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader, VK_FORMAT_R32_UINT, VK_FORMAT_UNDEFINED);

        VkDescriptorSetLayout uboLayout = pipeline_->descriptorSetLayouts[0];
        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts = &uboLayout;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &perFrameUBODescriptorSet_);

        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = perFrameUBOBuffer_.buffer;
        bufInfo.offset = 0;
        bufInfo.range = uboSize;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = perFrameUBODescriptorSet_;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bufInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;

    VulkanBuffer perFrameUBOBuffer_{};
    VkDescriptorSet perFrameUBODescriptorSet_ = VK_NULL_HANDLE;
    VulkanPipeline* pipeline_ = nullptr;
};
