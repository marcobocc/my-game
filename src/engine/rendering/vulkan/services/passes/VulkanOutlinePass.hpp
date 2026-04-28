#pragma once
#include <string>
#include <vector>
#include <volk.h>
#include "../../../../assets/types/Shader.hpp"
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "core/GameWindow.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"
class VulkanOutlinePass {
public:
    VulkanOutlinePass(const VulkanContext& context,
                      AssetManager& assetManager,
                      VulkanResourcesManager& resourcesManager,
                      VkFormat swapchainImageFormat,
                      uint32_t frameCount,
                      uint32_t width,
                      uint32_t height) :
        context_(context),
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        width_(width),
        height_(height),
        swapchainImageFormat_(swapchainImageFormat) {
        initPipeline(frameCount);
    }

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
    }

    void enqueueForOutline(const Renderer& renderer, const Transform& transform, std::string objectId) {
        outlineQueue_.push_back({renderer, transform, std::move(objectId)});
    }

    void record(VkCommandBuffer cmd,
                uint32_t imageIndex,
                const VulkanSwapchain& swapchain,
                VkImageView objectIdImageView,
                VkSampler objectIdBufferSampler,
                const GameWindow& window,
                const std::vector<std::string>& objectIdMap) {
        if (outlineQueue_.empty()) return;
        uint32_t targetId = resolveTargetId(objectIdMap);
        if (targetId == 0) return;
        updateDescriptor(imageIndex, objectIdImageView, objectIdBufferSampler);
        recordCompositePass(cmd, imageIndex, swapchain, window, targetId);
        outlineQueue_.clear();
    }

private:
    void updateDescriptor(uint32_t imageIndex, VkImageView objectIdImageView, VkSampler objectIdBufferSampler) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = objectIdBufferSampler;
        imageInfo.imageView = objectIdImageView;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = descriptorSets_[imageIndex];
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
    }

    uint32_t resolveTargetId(const std::vector<std::string>& objectIdMap) const {
        const auto& targetObjectId = outlineQueue_.front().objectId;
        for (uint32_t i = 0; i < static_cast<uint32_t>(objectIdMap.size()); ++i) {
            if (objectIdMap[i] == targetObjectId) return i + 1;
        }
        return 0;
    }

    void recordCompositePass(VkCommandBuffer cmd,
                             uint32_t imageIndex,
                             const VulkanSwapchain& swapchain,
                             const GameWindow& window,
                             uint32_t targetId) {
        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchain.swapchainImageViews[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, swapchain.swapchainExtent};
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

        const float fw = static_cast<float>(width_);
        const float fh = static_cast<float>(height_);
        struct OutlinePush {
            glm::vec2 texelSize;
            glm::vec2 uvOffset;
            glm::vec2 uvScale;
            uint32_t targetId;
        } push;
        push.texelSize = {1.0f / fw, 1.0f / fh};
        push.uvOffset = {static_cast<float>(fbX) / fw, static_cast<float>(fbY) / fh};
        push.uvScale = {static_cast<float>(fbW) / fw, static_cast<float>(fbH) / fh};
        push.targetId = targetId;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeline_->layout,
                                0,
                                1,
                                &descriptorSets_[imageIndex],
                                0,
                                nullptr);
        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(OutlinePush),
                           &push);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

    void initPipeline(uint32_t frameCount) {
        const Shader* shader = assetManager_.get<Shader>(OUTLINE_SHADER);
        if (!shader) return;

        pipeline_ = &resourcesManager_.getPipeline(*shader, swapchainImageFormat_, VK_FORMAT_UNDEFINED);
        VkDescriptorSetLayout samplerLayout = pipeline_->descriptorSetLayouts[0];

        descriptorSets_.resize(frameCount);
        std::vector<VkDescriptorSetLayout> layouts(frameCount, samplerLayout);

        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = frameCount;
        dsAlloc.pSetLayouts = layouts.data();
        vkAllocateDescriptorSets(context_.device, &dsAlloc, descriptorSets_.data());
    }

    const VulkanContext& context_;
    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;

    std::vector<VkDescriptorSet> descriptorSets_;
    VulkanPipeline* pipeline_ = nullptr;
    std::vector<DrawCall> outlineQueue_;
};
