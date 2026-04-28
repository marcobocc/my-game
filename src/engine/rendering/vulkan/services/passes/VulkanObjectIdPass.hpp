#pragma once
#include <string>
#include <vector>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "assets/types/Mesh.hpp"
#include "assets/types/Shader.hpp"
#include "core/GameWindow.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/buffers.hpp"
#include "rendering/vulkan/core/memory.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

class VulkanObjectIdPass {
public:
    VulkanObjectIdPass(const VulkanContext& context,
                       AssetManager& assetManager,
                       VulkanResourcesManager& resourcesManager,
                       VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        width_(swapchainManager.swapchain().swapchainExtent.width),
        height_(swapchainManager.swapchain().swapchainExtent.height) {
        createImages();
        initPipeline();
    }

    ~VulkanObjectIdPass() { destroyImages(); }

    void resize(uint32_t width, uint32_t height) {
        vkDeviceWaitIdle(context_.device);
        destroyImages();
        width_ = width;
        height_ = height;
        createImages();
        updateDescriptorSet();
    }

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    const std::vector<std::string>& getObjectIdMap() const { return objectIdMap_; }

    // Accessors for the outline pass to reuse the object id buffer
    VkImage objectIdBufferImage() const { return objectIdBufferImage_; }
    VkImageView objectIdBufferImageView() const { return objectIdBufferImageView_; }
    VkSampler objectIdBufferSampler() const { return objectIdBufferSampler_; }

    void record(VkCommandBuffer cmd,
                const std::vector<DrawCall>& drawQueue,
                const Camera& camera,
                const Transform& cameraTransform,
                const GameWindow& window) {
        if (pipeline_ == nullptr) return;

        transitionImage(cmd,
                        objectIdBufferImage_,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        0,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT);

        transitionImage(cmd,
                        depthImage_,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        0,
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        VK_IMAGE_ASPECT_DEPTH_BIT);

        VkClearValue clearObjectId{};
        clearObjectId.color.uint32[0] = 0xFFFFFFFF;
        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = objectIdBufferImageView_;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearObjectId;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthImageView_;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.clearValue = clearDepth;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, {width_, height_}};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);

        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->layout, 0, 1, &perFrameUBODescriptorSet_, 0, nullptr);

        glm::mat4 uboData[2] = {cameraTransform.getViewMatrix(), camera.getProjectionMatrix()};
        updateBuffer(context_.device, perFrameUBOBuffer_, uboData, sizeof(uboData));

        objectIdMap_.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(drawQueue.size()); ++i) {
            const DrawCall& dc = drawQueue[i];
            objectIdMap_.push_back(dc.objectId);

            struct ObjectIdPush {
                glm::mat4 model;
                uint32_t objectId;
            } push;
            push.model = dc.transform.getModelMatrix();
            push.objectId = i + 1; // 0 = no object

            vkCmdPushConstants(cmd,
                               pipeline_->layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(ObjectIdPush),
                               &push);

            const Mesh* mesh = assetManager_.get<Mesh>(dc.renderer.meshName);
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

        // Transition to TRANSFER_SRC so the picking backend can copy a pixel if needed,
        // then leave in SHADER_READ_ONLY_OPTIMAL for the outline pass to sample.
        transitionImage(cmd,
                        objectIdBufferImage_,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT);
    }

private:
    void createImages() {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {width_, height_, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R32_UINT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage =
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(context_.device, &imageInfo, nullptr, &objectIdBufferImage_);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(context_.device, objectIdBufferImage_, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &objectIdBufferMemory_);
        vkBindImageMemory(context_.device, objectIdBufferImage_, objectIdBufferMemory_, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = objectIdBufferImage_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32_UINT;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(context_.device, &viewInfo, nullptr, &objectIdBufferImageView_);

        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        vkCreateImage(context_.device, &imageInfo, nullptr, &depthImage_);

        vkGetImageMemoryRequirements(context_.device, depthImage_, &memReq);
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &depthMemory_);
        vkBindImageMemory(context_.device, depthImage_, depthMemory_, 0);

        viewInfo.image = depthImage_;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vkCreateImageView(context_.device, &viewInfo, nullptr, &depthImageView_);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(context_.device, &samplerInfo, nullptr, &objectIdBufferSampler_);

        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        perFrameUBOBuffer_ = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);
    }

    void initPipeline() {
        const Shader* shader = assetManager_.get<Shader>(OBJECT_ID_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader, VK_FORMAT_R32_UINT, VK_FORMAT_D32_SFLOAT);
        updateDescriptorSet();
    }

    void updateDescriptorSet() {
        if (pipeline_ == nullptr) return;

        if (perFrameUBODescriptorSet_ == VK_NULL_HANDLE) {
            VkDescriptorSetLayout uboLayout = pipeline_->descriptorSetLayouts[0];
            VkDescriptorSetAllocateInfo dsAlloc{};
            dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            dsAlloc.descriptorPool = context_.descriptorPool;
            dsAlloc.descriptorSetCount = 1;
            dsAlloc.pSetLayouts = &uboLayout;
            vkAllocateDescriptorSets(context_.device, &dsAlloc, &perFrameUBODescriptorSet_);
        }

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

    void destroyImages() {
        if (objectIdBufferSampler_ != VK_NULL_HANDLE)
            vkDestroySampler(context_.device, objectIdBufferSampler_, nullptr);
        if (objectIdBufferImageView_ != VK_NULL_HANDLE)
            vkDestroyImageView(context_.device, objectIdBufferImageView_, nullptr);
        if (objectIdBufferImage_ != VK_NULL_HANDLE) vkDestroyImage(context_.device, objectIdBufferImage_, nullptr);
        if (objectIdBufferMemory_ != VK_NULL_HANDLE) vkFreeMemory(context_.device, objectIdBufferMemory_, nullptr);
        if (depthImageView_ != VK_NULL_HANDLE) vkDestroyImageView(context_.device, depthImageView_, nullptr);
        if (depthImage_ != VK_NULL_HANDLE) vkDestroyImage(context_.device, depthImage_, nullptr);
        if (depthMemory_ != VK_NULL_HANDLE) vkFreeMemory(context_.device, depthMemory_, nullptr);
        destroyBuffer(context_.device, perFrameUBOBuffer_);
        objectIdBufferSampler_ = VK_NULL_HANDLE;
        objectIdBufferImageView_ = VK_NULL_HANDLE;
        objectIdBufferImage_ = VK_NULL_HANDLE;
        objectIdBufferMemory_ = VK_NULL_HANDLE;
        depthImageView_ = VK_NULL_HANDLE;
        depthImage_ = VK_NULL_HANDLE;
        depthMemory_ = VK_NULL_HANDLE;
        perFrameUBODescriptorSet_ = VK_NULL_HANDLE;
    }

    static void transitionImage(VkCommandBuffer cmd,
                                VkImage image,
                                VkImageLayout oldLayout,
                                VkImageLayout newLayout,
                                VkAccessFlags srcAccess,
                                VkAccessFlags dstAccess,
                                VkPipelineStageFlags srcStage,
                                VkPipelineStageFlags dstStage,
                                VkImageAspectFlags aspect) {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcAccessMask = srcAccess;
        barrier.dstAccessMask = dstAccess;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange = {aspect, 0, 1, 0, 1};
        vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    const VulkanContext& context_;
    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;

    uint32_t width_ = 0;
    uint32_t height_ = 0;

    VkImage objectIdBufferImage_ = VK_NULL_HANDLE;
    VkDeviceMemory objectIdBufferMemory_ = VK_NULL_HANDLE;
    VkImageView objectIdBufferImageView_ = VK_NULL_HANDLE;
    VkSampler objectIdBufferSampler_ = VK_NULL_HANDLE;

    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;

    VulkanBuffer perFrameUBOBuffer_{};
    VkDescriptorSet perFrameUBODescriptorSet_ = VK_NULL_HANDLE;

    VulkanPipeline* pipeline_ = nullptr;

    std::vector<std::string> objectIdMap_;
};
