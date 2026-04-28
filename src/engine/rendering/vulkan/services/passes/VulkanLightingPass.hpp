#pragma once
#include <vector>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "assets/types/Shader.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"

class VulkanLightingPass {
public:
    VulkanLightingPass(const VulkanContext& context,
                       AssetManager& assetManager,
                       VulkanResourcesManager& resourcesManager,
                       VkFormat swapchainImageFormat,
                       uint32_t frameCount) :
        context_(context),
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        swapchainImageFormat_(swapchainImageFormat) {
        initPipeline(frameCount);
    }

    void record(VkCommandBuffer cmd,
                uint32_t imageIndex,
                VkImageView swapchainColorView,
                VkExtent2D swapchainExtent,
                VkImageView albedoView,
                VkSampler albedoSampler,
                VkImageView normalView,
                VkSampler normalSampler,
                int fbX,
                int fbY,
                int fbW,
                int fbH) {
        if (pipeline_ == nullptr) return;
        updateDescriptor(imageIndex, albedoView, albedoSampler, normalView, normalSampler);

        VkClearValue clearColor{};
        clearColor.color = {{0.1f, 0.1f, 0.1f, 1.0f}};

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchainColorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearColor;

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

        const float scW = static_cast<float>(swapchainExtent.width);
        const float scH = static_cast<float>(swapchainExtent.height);
        struct LightingPush {
            glm::vec2 uvOffset;
            glm::vec2 uvScale;
        } push{};
        push.uvOffset = {static_cast<float>(fbX) / scW, static_cast<float>(fbY) / scH};
        push.uvScale = {static_cast<float>(fbW) / scW, static_cast<float>(fbH) / scH};

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
                           sizeof(LightingPush),
                           &push);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

private:
    void updateDescriptor(uint32_t imageIndex,
                          VkImageView albedoView,
                          VkSampler albedoSampler,
                          VkImageView normalView,
                          VkSampler normalSampler) {
        VkDescriptorImageInfo imageInfos[2] = {};
        imageInfos[0].sampler = albedoSampler;
        imageInfos[0].imageView = albedoView;
        imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[1].sampler = normalSampler;
        imageInfos[1].imageView = normalView;
        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writes[2] = {};
        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = descriptorSets_[imageIndex];
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &imageInfos[0];

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSets_[imageIndex];
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &imageInfos[1];
        vkUpdateDescriptorSets(context_.device, 2, writes, 0, nullptr);
    }

    void initPipeline(uint32_t frameCount) {
        const Shader* shader = assetManager_.get<Shader>(LIGHTING_SHADER);
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

    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    VulkanPipeline* pipeline_ = nullptr;
    std::vector<VkDescriptorSet> descriptorSets_;
};
