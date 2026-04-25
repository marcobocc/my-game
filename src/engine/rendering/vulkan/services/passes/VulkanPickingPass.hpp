#pragma once
#include <optional>
#include <string>
#include <vector>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "assets/types/mesh/Mesh.hpp"
#include "assets/types/mesh/details/VertexLayouts.hpp"
#include "assets/types/shader/Shader.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/buffers.hpp"
#include "rendering/vulkan/core/memory.hpp"
#include "rendering/vulkan/core/shaders.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

class VulkanPickingPass {
public:
    VulkanPickingPass(const VulkanContext& context,
                      AssetManager& assetManager,
                      VulkanResourcesManager& resourcesManager,
                      VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        width_(swapchainManager.swapchain().swapchainExtent.width),
        height_(swapchainManager.swapchain().swapchainExtent.height) {
        createImages();
        createPipeline();
        createReadbackBuffer();
    }

    ~VulkanPickingPass() { destroyResources(); }

    void resize(uint32_t width, uint32_t height) {
        vkDeviceWaitIdle(context_.device);
        destroyImages();
        width_ = width;
        height_ = height;
        createImages();
        if (readbackBuffer_.buffer != VK_NULL_HANDLE) {
            destroyBuffer(context_.device, readbackBuffer_);
            createReadbackBuffer();
        }
    }

    void requestPick(uint32_t x, uint32_t y) {
        hasPendingPickRequest_ = true;
        pendingPickX_ = x;
        pendingPickY_ = y;
    }

    std::optional<std::string> getPickResult() { return std::exchange(pendingPickResult_, std::nullopt); }

    void record(VkCommandBuffer cmd,
                const std::vector<DrawCall>& drawQueue,
                const Camera& camera,
                const Transform& cameraTransform) {
        if (pipeline_.pipeline == VK_NULL_HANDLE) return;

        transitionImage(cmd,
                        colorImage_,
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

        VkClearValue clearId{};
        clearId.color.uint32[0] = 0xFFFFFFFF;
        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorImageView_;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearId;

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

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = static_cast<float>(height_);
        viewport.width = static_cast<float>(width_);
        viewport.height = -static_cast<float>(height_);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{0, 0}, {width_, height_}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.layout, 0, 1, &perFrameUBODescriptorSet_, 0, nullptr);

        glm::mat4 uboData[2] = {cameraTransform.getViewMatrix(), camera.getProjectionMatrix()};
        updateBuffer(context_.device, perFrameUBOBuffer_, uboData, sizeof(uboData));

        objectIdMap_.clear();
        for (uint32_t i = 0; i < static_cast<uint32_t>(drawQueue.size()); ++i) {
            const DrawCall& dc = drawQueue[i];
            objectIdMap_.push_back(dc.objectId);

            struct PickPush {
                glm::mat4 model;
                uint32_t objectId;
            } push;
            push.model = dc.transform.getModelMatrix();
            push.objectId = i + 1; // 0 = no object

            vkCmdPushConstants(cmd,
                               pipeline_.layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(PickPush),
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

        transitionImage(cmd,
                        colorImage_,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_ACCESS_TRANSFER_READ_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_TRANSFER_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT);

        if (hasPendingPickRequest_) {
            uint32_t x = std::min(pendingPickX_, width_ - 1);
            uint32_t y = std::min(pendingPickY_, height_ - 1);

            VkBufferImageCopy region{};
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
            region.imageExtent = {1, 1, 1};
            vkCmdCopyImageToBuffer(
                    cmd, colorImage_, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readbackBuffer_.buffer, 1, &region);

            hasPendingPickRequest_ = false;
            pickResultPending_ = true;
        }
    }

    // Call after the frame fence is signaled. Resolves and returns the picked object ID if ready.
    void resolvePickResult() {
        if (!pickResultPending_) return;
        pickResultPending_ = false;

        uint32_t id = 0;
        void* mapped = nullptr;
        vkMapMemory(context_.device, readbackBuffer_.memory, 0, sizeof(uint32_t), 0, &mapped);
        std::memcpy(&id, mapped, sizeof(uint32_t));
        vkUnmapMemory(context_.device, readbackBuffer_.memory);

        if (id == 0 || id == 0xFFFFFFFF || (id - 1) >= objectIdMap_.size()) {
            pendingPickResult_ = std::nullopt;
            return;
        }
        pendingPickResult_ = objectIdMap_[id - 1];
    }

    bool hasPickResultPending() const { return pickResultPending_; }

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
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(context_.device, &imageInfo, nullptr, &colorImage_);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(context_.device, colorImage_, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &colorMemory_);
        vkBindImageMemory(context_.device, colorImage_, colorMemory_, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = colorImage_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32_UINT;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(context_.device, &viewInfo, nullptr, &colorImageView_);

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

        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        perFrameUBOBuffer_ = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);

        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboBinding;
        vkCreateDescriptorSetLayout(context_.device, &layoutInfo, nullptr, &perFrameUBOLayout_);

        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts = &perFrameUBOLayout_;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &perFrameUBODescriptorSet_);

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

    void createPipeline() {
        const Shader* shader = assetManager_.get<Shader>(PICKING_SHADER);
        if (!shader) return;

        VkVertexInputBindingDescription vertexBinding{};
        vertexBinding.binding = 0;
        vertexBinding.stride = sizeof(float) * Vertex_WithLayout_PositionUv::VERTEX_STRIDE;
        vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::vector<VkVertexInputAttributeDescription> vertexAttribs;
        uint32_t location = 0;
        for (const auto& [offset, componentCount]: Vertex_WithLayout_PositionUv::VERTEX_ATTRIBS) {
            VkFormat format = VK_FORMAT_UNDEFINED;
            if (componentCount == 2) format = VK_FORMAT_R32G32_SFLOAT;
            if (componentCount == 3) format = VK_FORMAT_R32G32B32_SFLOAT;
            VkVertexInputAttributeDescription desc{};
            desc.location = location++;
            desc.binding = 0;
            desc.format = format;
            desc.offset = static_cast<uint32_t>(offset * sizeof(float));
            vertexAttribs.push_back(desc);
        }

        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.vertexBindingDescriptionCount = 1;
        vertexInputState.pVertexBindingDescriptions = &vertexBinding;
        vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttribs.size());
        vertexInputState.pVertexAttributeDescriptions = vertexAttribs.data();

        pipeline_ = createGraphicsPipeline(context_.device,
                                           *shader,
                                           vertexInputState,
                                           {perFrameUBOLayout_},
                                           VK_FORMAT_R32_UINT,
                                           VK_FORMAT_D32_SFLOAT);
    }

    void createReadbackBuffer() {
        readbackBuffer_ = createBuffer(context_.device,
                                       context_.physicalDevice,
                                       sizeof(uint32_t),
                                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }

    void destroyImages() {
        if (colorImageView_ != VK_NULL_HANDLE) vkDestroyImageView(context_.device, colorImageView_, nullptr);
        if (colorImage_ != VK_NULL_HANDLE) vkDestroyImage(context_.device, colorImage_, nullptr);
        if (colorMemory_ != VK_NULL_HANDLE) vkFreeMemory(context_.device, colorMemory_, nullptr);
        if (depthImageView_ != VK_NULL_HANDLE) vkDestroyImageView(context_.device, depthImageView_, nullptr);
        if (depthImage_ != VK_NULL_HANDLE) vkDestroyImage(context_.device, depthImage_, nullptr);
        if (depthMemory_ != VK_NULL_HANDLE) vkFreeMemory(context_.device, depthMemory_, nullptr);
        if (perFrameUBOLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, perFrameUBOLayout_, nullptr);
        destroyBuffer(context_.device, perFrameUBOBuffer_);
        colorImageView_ = VK_NULL_HANDLE;
        colorImage_ = VK_NULL_HANDLE;
        colorMemory_ = VK_NULL_HANDLE;
        depthImageView_ = VK_NULL_HANDLE;
        depthImage_ = VK_NULL_HANDLE;
        depthMemory_ = VK_NULL_HANDLE;
        perFrameUBOLayout_ = VK_NULL_HANDLE;
        perFrameUBODescriptorSet_ = VK_NULL_HANDLE;
    }

    void destroyResources() {
        destroyImages();
        if (pipeline_.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(context_.device, pipeline_.pipeline, nullptr);
        if (pipeline_.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(context_.device, pipeline_.layout, nullptr);
        destroyBuffer(context_.device, readbackBuffer_);
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

    VkImage colorImage_ = VK_NULL_HANDLE;
    VkDeviceMemory colorMemory_ = VK_NULL_HANDLE;
    VkImageView colorImageView_ = VK_NULL_HANDLE;

    VkImage depthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory_ = VK_NULL_HANDLE;
    VkImageView depthImageView_ = VK_NULL_HANDLE;

    VulkanBuffer perFrameUBOBuffer_{};
    VkDescriptorSetLayout perFrameUBOLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet perFrameUBODescriptorSet_ = VK_NULL_HANDLE;

    VulkanPipeline pipeline_{};
    VulkanBuffer readbackBuffer_{};

    std::vector<std::string> objectIdMap_;

    bool hasPendingPickRequest_ = false;
    uint32_t pendingPickX_ = 0;
    uint32_t pendingPickY_ = 0;
    bool pickResultPending_ = false;
    std::optional<std::string> pendingPickResult_;
};
