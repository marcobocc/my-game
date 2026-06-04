#pragma once
#include <array>
#include <cassert>
#include <cmath>
#include <cstring>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <volk.h>
#include "../../../asset_management/asset_types/Shader.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/descriptors.hpp"
#include "../core/utils/structs.hpp"
#include "VulkanShadowPass.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Light.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanLightingPass {
public:
    // Number of vec4s each light occupies in the light data region. Must match the shader.
    static constexpr uint32_t LIGHT_STRIDE = 4;

    // 32-byte header prepended to every per-slot SSBO.
    // Layout matches the shader's `uvec4 header[2]` declaration.
    struct LightsSSBOHeader {
        uint32_t lightIndex;
        uint32_t isFirstLight;
        uint32_t lightCount;
        uint32_t _pad;
        uint32_t _reserved[4];
    };
    static_assert(sizeof(LightsSSBOHeader) == 32);

    VulkanLightingPass(const VulkanContext& context,
                       AssetLoader& assetLoader,
                       VulkanResourcesManager& resourcesManager,
                       VkFormat swapchainImageFormat) :
        context_(context),
        assetLoader_(assetLoader),
        resourcesManager_(resourcesManager) {
        initPipeline(swapchainImageFormat);
    }

    ~VulkanLightingPass() {
        for (auto& buf: lightsBuffers_)
            if (buf.buffer) destroyBuffer(context_.device, buf);
    }

    // Allocate a descriptor set bound to slot `lightSlot`'s SSBO.
    VkDescriptorSet createDescriptorSet(uint32_t lightSlot) {
        assert(lightSlot < VulkanShadowPass::MAX_SHADOW_LIGHTS);
        if (!pipeline_) return VK_NULL_HANDLE;
        VkDescriptorSetLayout layout = pipeline_->descriptorSetLayouts[0];
        VkDescriptorSet ds = VK_NULL_HANDLE;
        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts = &layout;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &ds);
        cachedDescriptorSetsBySlot_[lightSlot].push_back(ds);
        ensureSlotBuffer(lightSlot);
        updateLightsDescriptor(ds, lightSlot);
        return ds;
    }

    // Write all light data into every per-slot SSBO (light data region only, not the header).
    // Call once per frame before any record() calls.
    void updateLights(const std::vector<std::pair<Light, Transform>>& lightsWithTransforms) {
        lightsCount_ = lightsWithTransforms.size();

        std::vector<glm::vec4> lightData;
        lightData.reserve(lightsWithTransforms.size() * LIGHT_STRIDE);
        for (const auto& [light, transform]: lightsWithTransforms) {
            const glm::vec3 direction = -transform.getForward();
            const float type = static_cast<float>(light.type);
            const float cosInner = std::cos(glm::radians(light.innerConeAngle));
            const float cosOuter = std::cos(glm::radians(light.outerConeAngle));
            lightData.emplace_back(direction, type);
            lightData.emplace_back(light.color, light.intensity);
            lightData.emplace_back(transform.position, 0.0f);
            lightData.emplace_back(cosInner, cosOuter, light.range, 0.0f);
        }

        const VkDeviceSize headerSize = sizeof(LightsSSBOHeader);
        const VkDeviceSize dataSize = lightData.size() * sizeof(glm::vec4);
        const VkDeviceSize totalSize = headerSize + (dataSize > 0 ? dataSize : sizeof(glm::vec4));

        for (uint32_t slot = 0; slot < VulkanShadowPass::MAX_SHADOW_LIGHTS; ++slot) {
            auto& buf = lightsBuffers_[slot];
            bool recreated = false;
            if (!buf.buffer || buf.allocSize < totalSize) {
                if (buf.buffer) destroyBuffer(context_.device, buf);
                buf = createBuffer(context_.device,
                                   context_.physicalDevice,
                                   totalSize,
                                   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                recreated = true;
            }
            if (dataSize > 0) updateBuffer(context_.device, buf, lightData.data(), dataSize, headerSize);

            if (recreated) {
                auto it = cachedDescriptorSetsBySlot_.find(slot);
                if (it != cachedDescriptorSetsBySlot_.end())
                    for (auto ds: it->second)
                        updateLightsDescriptor(ds, slot);
            }
        }
    }

    // Write the 32-byte header for a specific slot. Call just before record() for that slot.
    void uploadPassHeader(uint32_t lightSlot, bool isFirstLight) const {
        assert(lightSlot < VulkanShadowPass::MAX_SHADOW_LIGHTS);
        ensureSlotBuffer(lightSlot);
        LightsSSBOHeader hdr{};
        hdr.lightIndex = lightSlot;
        hdr.isFirstLight = isFirstLight ? 1u : 0u;
        hdr.lightCount = static_cast<uint32_t>(lightsCount_);
        updateBuffer(context_.device, lightsBuffers_[lightSlot], &hdr, sizeof(hdr), 0);
    }

    void record(VkCommandBuffer cmd,
                VkDescriptorSet descriptorSet,
                VkImageView swapchainColorView,
                VkExtent2D swapchainExtent,
                VkImageView albedoView,
                VkSampler albedoSampler,
                VkImageView normalView,
                VkSampler normalSampler,
                VkImageView shadowMapView,
                VkSampler shadowMapSampler,
                VkImageView gbufferDepthView,
                VkSampler gbufferDepthSampler,
                const glm::mat4& lightSpaceMatrix,
                const Camera& camera,
                const Transform& cameraTransform,
                int fbX,
                int fbY,
                int fbW,
                int fbH,
                float gbufferWidth,
                float gbufferHeight,
                bool enableLighting,
                bool isFirstLight) {
        VulkanPipeline* activePipeline = isFirstLight ? pipeline_ : pipelineAdditive_;
        if (activePipeline == nullptr) return;

        updateDescriptor(descriptorSet,
                         albedoView,
                         albedoSampler,
                         normalView,
                         normalSampler,
                         shadowMapView,
                         shadowMapSampler,
                         gbufferDepthView,
                         gbufferDepthSampler);

        VkClearValue clearBlack{};
        clearBlack.color = {{0.0f, 0.0f, 0.0f, 1.0f}};

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchainColorView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = isFirstLight ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearBlack;

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
            glm::mat4 lightSpaceFromScreen;
            glm::mat4 invViewProj;
            glm::vec2 uvOffset;
            glm::vec2 uvScale;
            uint32_t enableLighting;
            uint32_t _pad;
        } push{};
        const glm::mat4 invViewProj = glm::inverse(camera.getProjectionMatrix() * cameraTransform.getViewMatrix());
        push.lightSpaceFromScreen = lightSpaceMatrix * invViewProj;
        push.invViewProj = invViewProj;
        push.uvOffset = {static_cast<float>(fbX) / scW, static_cast<float>(fbY) / scH};
        push.uvScale = {static_cast<float>(fbW) / scW, static_cast<float>(fbH) / scH};
        push.enableLighting = enableLighting ? 1 : 0;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline->pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline->layout, 0, 1, &descriptorSet, 0, nullptr);
        vkCmdPushConstants(cmd,
                           activePipeline->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(LightingPush),
                           &push);
        vkCmdDraw(cmd, 3, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

private:
    void ensureSlotBuffer(uint32_t slot) const {
        auto& buf = lightsBuffers_[slot];
        if (buf.buffer) return;
        // Minimal allocation: just the header + space for one light (for the DS binding).
        const VkDeviceSize minSize = sizeof(LightsSSBOHeader) + LIGHT_STRIDE * sizeof(glm::vec4);
        buf = createBuffer(context_.device,
                           context_.physicalDevice,
                           minSize,
                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void updateDescriptor(VkDescriptorSet descriptorSet,
                          VkImageView albedoView,
                          VkSampler albedoSampler,
                          VkImageView normalView,
                          VkSampler normalSampler,
                          VkImageView shadowMapView,
                          VkSampler shadowMapSampler,
                          VkImageView gbufferDepthView,
                          VkSampler gbufferDepthSampler) {
        VkDescriptorImageInfo imageInfos[4] = {};
        imageInfos[0].sampler = albedoSampler;
        imageInfos[0].imageView = albedoView;
        imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[1].sampler = normalSampler;
        imageInfos[1].imageView = normalView;
        imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[2].sampler = shadowMapSampler;
        imageInfos[2].imageView = shadowMapView;
        imageInfos[2].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfos[3].sampler = gbufferDepthSampler;
        imageInfos[3].imageView = gbufferDepthView;
        imageInfos[3].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet writes[4] = {};
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

        writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[2].dstSet = descriptorSet;
        writes[2].dstBinding = 3;
        writes[2].descriptorCount = 1;
        writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[2].pImageInfo = &imageInfos[2];

        writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[3].dstSet = descriptorSet;
        writes[3].dstBinding = 4;
        writes[3].descriptorCount = 1;
        writes[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[3].pImageInfo = &imageInfos[3];

        vkUpdateDescriptorSets(context_.device, 4, writes, 0, nullptr);
    }

    void updateLightsDescriptor(VkDescriptorSet ds, uint32_t slot) {
        ensureSlotBuffer(slot);
        const auto& buf = lightsBuffers_[slot];

        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = buf.buffer;
        bufInfo.offset = 0;
        bufInfo.range = buf.allocSize;

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
        pipeline_ = &resourcesManager_.getPipeline(*shader,
                                                   swapchainImageFormat,
                                                   VK_FORMAT_UNDEFINED,
                                                   /*cullFront=*/false,
                                                   /*additiveBlend=*/false);
        pipelineAdditive_ = &resourcesManager_.getPipeline(*shader,
                                                           swapchainImageFormat,
                                                           VK_FORMAT_UNDEFINED,
                                                           /*cullFront=*/false,
                                                           /*additiveBlend=*/true);
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;

    VulkanPipeline* pipeline_ = nullptr;
    VulkanPipeline* pipelineAdditive_ = nullptr;

    mutable std::array<VulkanBuffer, VulkanShadowPass::MAX_SHADOW_LIGHTS> lightsBuffers_{};
    mutable size_t lightsCount_ = 0;
    std::unordered_map<uint32_t, std::vector<VkDescriptorSet>> cachedDescriptorSetsBySlot_;
};
