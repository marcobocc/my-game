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
                int fbX,
                int fbY,
                int fbW,
                int fbH) {
        if (pipeline_ == nullptr) return;
        updateDescriptor(imageIndex, albedoView, albedoSampler);

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

        // Match GeometryPass: Y-flipped viewport restricted to the scene region so
        // that UV (0,0) in the lighting shader maps to the same pixel GeometryPass
        // wrote to — without this the gbuffer is sampled with both axes inverted.
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

        // The gbuffer is full swapchain-sized; scene content lives in the
        // [fbX,fbY,fbW,fbH] sub-region.  Push a UV offset+scale so the
        // fullscreen triangle samples only that sub-region.
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
        if (pipeline_->pushConstantSize >= sizeof(LightingPush)) {
            vkCmdPushConstants(cmd,
                               pipeline_->layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(LightingPush),
                               &push);
        }
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

private:
    void updateDescriptor(uint32_t imageIndex, VkImageView albedoView, VkSampler albedoSampler) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = albedoSampler;
        imageInfo.imageView = albedoView;
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
