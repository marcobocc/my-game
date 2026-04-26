#pragma once
#include <string>
#include <vector>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "assets/types/shader/Shader.hpp"
#include "rendering/vulkan/core/shaders.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

class VulkanOutlinePass {
public:
    VulkanOutlinePass(const VulkanContext& context,
                      AssetManager& assetManager,
                      VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetManager_(assetManager),
        width_(swapchainManager.swapchain().swapchainExtent.width),
        height_(swapchainManager.swapchain().swapchainExtent.height),
        swapchainImageFormat_(swapchainManager.swapchain().swapchainImageFormat) {
        const uint32_t frameCount = static_cast<uint32_t>(swapchainManager.swapchain().swapchainImages.size());
        createDescriptorLayout(frameCount);
        createPipeline();
    }

    ~VulkanOutlinePass() { destroyResources(); }

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
    }

    void enqueueForOutline(const Renderer& renderer, const Transform& transform, std::string objectId) {
        outlineQueue_.push_back({renderer, transform, std::move(objectId)});
    }

    // objectIdBufferImageView and objectIdBufferSampler come from VulkanObjectIdPass
    void record(VkCommandBuffer cmd,
                uint32_t imageIndex,
                const VulkanSwapchain& swapchain,
                VkImageView objectIdBufferImageView,
                VkSampler objectIdBufferSampler) {
        if (outlineQueue_.empty()) return;

        updateDescriptor(imageIndex, objectIdBufferImageView, objectIdBufferSampler);
        recordCompositePass(cmd, imageIndex, swapchain);

        outlineQueue_.clear();
    }

private:
    void updateDescriptor(uint32_t imageIndex, VkImageView objectIdBufferImageView, VkSampler objectIdBufferSampler) {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = objectIdBufferSampler;
        imageInfo.imageView = objectIdBufferImageView;
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

    void recordCompositePass(VkCommandBuffer cmd, uint32_t imageIndex, const VulkanSwapchain& swapchain) {
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

        struct OutlinePush {
            glm::vec2 texelSize;
        } push;
        push.texelSize = {1.0f / static_cast<float>(width_), 1.0f / static_cast<float>(height_)};

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout, 0, 1, &descriptorSets_[imageIndex], 0, nullptr);
        vkCmdPushConstants(cmd,
                           pipeline_.layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(OutlinePush),
                           &push);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

    void createDescriptorLayout(uint32_t frameCount) {
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &samplerBinding;
        vkCreateDescriptorSetLayout(context_.device, &layoutInfo, nullptr, &descriptorLayout_);

        descriptorSets_.resize(frameCount);
        std::vector<VkDescriptorSetLayout> layouts(frameCount, descriptorLayout_);

        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = frameCount;
        dsAlloc.pSetLayouts = layouts.data();
        vkAllocateDescriptorSets(context_.device, &dsAlloc, descriptorSets_.data());
    }

    void createPipeline() {
        const Shader* shader = assetManager_.get<Shader>(OUTLINE_SHADER);
        if (!shader) return;

        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        pipeline_ = createGraphicsPipeline(context_.device,
                                           *shader,
                                           vertexInputState,
                                           {descriptorLayout_},
                                           swapchainImageFormat_,
                                           VK_FORMAT_UNDEFINED);
    }

    void destroyResources() {
        if (descriptorLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, descriptorLayout_, nullptr);
        if (pipeline_.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(context_.device, pipeline_.pipeline, nullptr);
        if (pipeline_.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(context_.device, pipeline_.layout, nullptr);
    }

    const VulkanContext& context_;
    AssetManager& assetManager_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;

    VkDescriptorSetLayout descriptorLayout_ = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets_;

    VulkanPipeline pipeline_{};

    std::vector<DrawCall> outlineQueue_;
};
