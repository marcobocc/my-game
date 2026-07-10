#pragma once
#include <string>
#include <vector>
#include <volk.h>
#include "../../assets/Shader.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/structs.hpp"
#include "core/GameWindow.hpp"
#include "core/assets/AssetLoader.hpp"
#include "core/assets/BuiltinAssetNames.hpp"

class VulkanOutlinePass {
public:
    VulkanOutlinePass(const VulkanContext& context,
                      AssetLoader& assetLoader,
                      VulkanResourcesManager& resourcesManager,
                      VkFormat swapchainImageFormat) :
        context_(context),
        assetLoader_(assetLoader),
        resourcesManager_(resourcesManager) {
        initPipeline(swapchainImageFormat);
    }

    VkDescriptorSet createDescriptorSet() const {
        if (!pipeline_) return VK_NULL_HANDLE;
        VkDescriptorSetLayout layout = pipeline_->descriptorSetLayouts[0];
        VkDescriptorSet ds = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts = &layout;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &ds);
        return ds;
    }

    void record(VkCommandBuffer cmd,
                VkDescriptorSet descriptorSet,
                VkImageView swapchainColorView,
                VkExtent2D swapchainExtent,
                VkImageView objectIdImageView,
                VkSampler objectIdBufferSampler,
                const GameWindow& window,
                const std::vector<std::string>& objectIdMap,
                const std::vector<DrawCall>& outlineQueue,
                uint32_t width,
                uint32_t height) {
        if (outlineQueue.empty()) return;
        std::vector<uint32_t> targetIds;
        targetIds.reserve(outlineQueue.size());
        for (const auto& dc: outlineQueue) {
            uint32_t id = resolveObjectId(objectIdMap, dc.objectId);
            if (id != 0) targetIds.push_back(id);
        }
        if (targetIds.empty()) return;
        updateDescriptor(descriptorSet, objectIdImageView, objectIdBufferSampler);
        recordCompositePass(cmd, descriptorSet, swapchainColorView, swapchainExtent, window, targetIds, width, height);
    }

private:
    void
    updateDescriptor(VkDescriptorSet descriptorSet, VkImageView objectIdImageView, VkSampler objectIdBufferSampler) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = objectIdBufferSampler;
        imageInfo.imageView = objectIdImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSet;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
    }

    uint32_t resolveObjectId(const std::vector<std::string>& objectIdMap, const std::string& objectId) const {
        for (uint32_t i = 0; i < static_cast<uint32_t>(objectIdMap.size()); ++i) {
            if (objectIdMap[i] == objectId) return i + 1;
        }
        return 0;
    }

    void recordCompositePass(VkCommandBuffer cmd,
                             VkDescriptorSet descriptorSet,
                             VkImageView swapchainColorView,
                             VkExtent2D swapchainExtent,
                             const GameWindow& window,
                             const std::vector<uint32_t>& targetIds,
                             uint32_t width,
                             uint32_t height) {
        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchainColorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, swapchainExtent};
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

        const float fw = static_cast<float>(width);
        const float fh = static_cast<float>(height);
        static constexpr uint32_t MAX_OUTLINE_IDS = 16;
        struct OutlinePush {
            glm::vec2 texelSize;
            glm::vec2 uvOffset;
            glm::vec2 uvScale;
            uint32_t targetCount;
            uint32_t targetIds[MAX_OUTLINE_IDS];
        } push{};
        push.texelSize = {1.0f / fw, 1.0f / fh};
        push.uvOffset = {static_cast<float>(fbX) / fw, static_cast<float>(fbY) / fh};
        push.uvScale = {static_cast<float>(fbW) / fw, static_cast<float>(fbH) / fh};
        push.targetCount = static_cast<uint32_t>(std::min(targetIds.size(), static_cast<size_t>(MAX_OUTLINE_IDS)));
        for (uint32_t i = 0; i < push.targetCount; ++i)
            push.targetIds[i] = targetIds[i];

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->layout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(OutlinePush),
                           &push);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

    void initPipeline(VkFormat swapchainImageFormat) {
        const Shader* shader = assetLoader_.get<Shader>(OUTLINE_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader, swapchainImageFormat, VK_FORMAT_UNDEFINED);
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;

    VulkanPipeline* pipeline_ = nullptr;
};
