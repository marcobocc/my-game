#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <volk.h>
#include "../../../asset_management/asset_types/Shader.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/descriptors.hpp"
#include "../core/utils/structs.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanLightingPass {
public:
    VulkanLightingPass(const VulkanContext& context,
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
        updateLightsDescriptor(ds);
        return ds;
    }

    void updateLights(const std::vector<std::pair<Light, Transform>>& lightsWithTransforms) const {
        if (lightsWithTransforms.empty()) {
            lightsCount_ = 0;
            return;
        }

        std::vector<glm::vec4> lightData;
        for (const auto& [light, transform]: lightsWithTransforms) {
            glm::vec3 direction = -transform.getForward();
            lightData.push_back(glm::vec4(direction, light.intensity));
        }

        const size_t dataSize = lightData.size() * sizeof(glm::vec4);
        if (!lightsBuffer_.buffer || lightsBuffer_.allocSize < dataSize) {
            if (lightsBuffer_.buffer) destroyBuffer(context_.device, lightsBuffer_);
            lightsBuffer_ = createBuffer(context_.device,
                                         context_.physicalDevice,
                                         dataSize,
                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         lightData.data());
        } else {
            updateBuffer(context_.device, lightsBuffer_, lightData.data(), dataSize);
        }
        lightsCount_ = lightsWithTransforms.size();
    }

    void record(VkCommandBuffer cmd,
                VkDescriptorSet descriptorSet,
                VkImageView swapchainColorView,
                VkExtent2D swapchainExtent,
                VkImageView albedoView,
                VkSampler albedoSampler,
                VkImageView normalView,
                VkSampler normalSampler,
                int fbX,
                int fbY,
                int fbW,
                int fbH,
                float gbufferWidth,
                float gbufferHeight,
                bool enableLighting = true) {
        if (pipeline_ == nullptr) return;
        updateDescriptor(descriptorSet, albedoView, albedoSampler, normalView, normalSampler);

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

        const float scW = gbufferWidth;
        const float scH = gbufferHeight;
        struct LightingPush {
            glm::vec2 uvOffset;
            glm::vec2 uvScale;
            uint32_t enableLighting;
            uint32_t lightCount;
        } push{};
        push.uvOffset = {static_cast<float>(fbX) / scW, static_cast<float>(fbY) / scH};
        push.uvScale = {static_cast<float>(fbW) / scW, static_cast<float>(fbH) / scH};
        push.enableLighting = enableLighting ? 1 : 0;
        push.lightCount = static_cast<uint32_t>(lightsCount_);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->layout, 0, 1, &descriptorSet, 0, nullptr);
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
    void updateDescriptor(VkDescriptorSet descriptorSet,
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
        writes[0].dstSet = descriptorSet;
        writes[0].dstBinding = 0;
        writes[0].descriptorCount = 1;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[0].pImageInfo = &imageInfos[0];

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = descriptorSet;
        writes[1].dstBinding = 1;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &imageInfos[1];
        vkUpdateDescriptorSets(context_.device, 2, writes, 0, nullptr);
    }

    void updateLightsDescriptor(VkDescriptorSet ds) const {
        if (!lightsBuffer_.buffer) {
            if (lightsBuffer_.allocSize == 0) {
                lightsBuffer_ =
                        createBuffer(context_.device,
                                     context_.physicalDevice,
                                     sizeof(glm::vec4),
                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }

        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = lightsBuffer_.buffer;
        bufInfo.offset = 0;
        bufInfo.range = lightsBuffer_.allocSize;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = ds;
        write.dstBinding = 2;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo = &bufInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
    }

    void initPipeline(VkFormat swapchainImageFormat) {
        const Shader* shader = assetLoader_.get<Shader>(LIGHTING_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader, swapchainImageFormat, VK_FORMAT_UNDEFINED);
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;

    VulkanPipeline* pipeline_ = nullptr;
    mutable VulkanBuffer lightsBuffer_{};
    mutable size_t lightsCount_ = 0;
};
