#pragma once
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

class VulkanOutlinePass {
public:
    VulkanOutlinePass(const VulkanContext& context,
                      AssetManager& assetManager,
                      VulkanResourcesManager& resourcesManager,
                      VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        width_(swapchainManager.swapchain().swapchainExtent.width),
        height_(swapchainManager.swapchain().swapchainExtent.height),
        swapchainImageFormat_(swapchainManager.swapchain().swapchainImageFormat) {
        createIdImage();
        createIdPipeline();
        createCompositePipeline();
    }

    ~VulkanOutlinePass() { destroyResources(); }

    void resize(uint32_t width, uint32_t height) {
        vkDeviceWaitIdle(context_.device);
        destroyIdImage();
        width_ = width;
        height_ = height;
        createIdImage();
        rebuildCompositePipelineDescriptor();
    }

    void enqueueForOutline(const Renderer& renderer, const Transform& transform, std::string objectId) {
        outlineQueue_.push_back({renderer, transform, std::move(objectId)});
    }

    void record(VkCommandBuffer cmd,
                uint32_t imageIndex,
                const VulkanSwapchain& swapchain,
                const Camera& camera,
                const Transform& cameraTransform) {
        if (outlineQueue_.empty()) return;

        recordIdPass(cmd, camera, cameraTransform);
        transitionIdImageToShaderRead(cmd);
        recordCompositePass(cmd, imageIndex, swapchain);

        outlineQueue_.clear();
    }

private:
    // ---- ID pass ----

    void recordIdPass(VkCommandBuffer cmd, const Camera& camera, const Transform& cameraTransform) {
        transitionImage(cmd,
                        idImage_,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        0,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT);

        transitionImage(cmd,
                        idDepthImage_,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        0,
                        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                        VK_IMAGE_ASPECT_DEPTH_BIT);

        VkClearValue clearId{};
        clearId.color.uint32[0] = 0;
        VkClearValue clearDepth{};
        clearDepth.depthStencil = {1.0f, 0};

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = idImageView_;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearId;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = idDepthImageView_;
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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, idPipeline_.pipeline);
        vkCmdBindDescriptorSets(
                cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, idPipeline_.layout, 0, 1, &idUBODescriptorSet_, 0, nullptr);

        glm::mat4 uboData[2] = {cameraTransform.getViewMatrix(), camera.getProjectionMatrix()};
        updateBuffer(context_.device, idUBOBuffer_, uboData, sizeof(uboData));

        for (uint32_t i = 0; i < static_cast<uint32_t>(outlineQueue_.size()); ++i) {
            const DrawCall& dc = outlineQueue_[i];

            struct IdPush {
                glm::mat4 model;
                uint32_t objectId;
            } push;
            push.model = dc.transform.getModelMatrix();
            push.objectId = i + 1;

            vkCmdPushConstants(cmd,
                               idPipeline_.layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               sizeof(IdPush),
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
    }

    void transitionIdImageToShaderRead(VkCommandBuffer cmd) {
        transitionImage(cmd,
                        idImage_,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                        VK_ACCESS_SHADER_READ_BIT,
                        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                        VK_IMAGE_ASPECT_COLOR_BIT);
    }

    // ---- Composite pass ----

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

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, compositePipeline_.pipeline);
        vkCmdBindDescriptorSets(cmd,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                compositePipeline_.layout,
                                0,
                                1,
                                &compositeDescriptorSet_,
                                0,
                                nullptr);
        vkCmdPushConstants(cmd,
                           compositePipeline_.layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(OutlinePush),
                           &push);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

    // ---- Resource creation ----

    void createIdImage() {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = {width_, height_, 1};
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = VK_FORMAT_R32_UINT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vkCreateImage(context_.device, &imageInfo, nullptr, &idImage_);

        VkMemoryRequirements memReq{};
        vkGetImageMemoryRequirements(context_.device, idImage_, &memReq);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &idMemory_);
        vkBindImageMemory(context_.device, idImage_, idMemory_, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = idImage_;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_R32_UINT;
        viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        vkCreateImageView(context_.device, &viewInfo, nullptr, &idImageView_);

        // depth image
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        vkCreateImage(context_.device, &imageInfo, nullptr, &idDepthImage_);

        vkGetImageMemoryRequirements(context_.device, idDepthImage_, &memReq);
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex =
                getMemoryType(context_.physicalDevice, memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(context_.device, &allocInfo, nullptr, &idDepthMemory_);
        vkBindImageMemory(context_.device, idDepthImage_, idDepthMemory_, 0);

        viewInfo.image = idDepthImage_;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        vkCreateImageView(context_.device, &viewInfo, nullptr, &idDepthImageView_);

        // sampler for composite pass
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        vkCreateSampler(context_.device, &samplerInfo, nullptr, &idSampler_);

        // UBO for id pass
        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        idUBOBuffer_ = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);

        VkDescriptorSetLayoutBinding uboBinding{};
        uboBinding.binding = 0;
        uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboBinding.descriptorCount = 1;
        uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo uboLayoutInfo{};
        uboLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        uboLayoutInfo.bindingCount = 1;
        uboLayoutInfo.pBindings = &uboBinding;
        vkCreateDescriptorSetLayout(context_.device, &uboLayoutInfo, nullptr, &idUBOLayout_);

        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts = &idUBOLayout_;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &idUBODescriptorSet_);

        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = idUBOBuffer_.buffer;
        bufInfo.offset = 0;
        bufInfo.range = uboSize;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = idUBODescriptorSet_;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bufInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);

        // composite descriptor set (sampled image)
        VkDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.binding = 0;
        samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerBinding.descriptorCount = 1;
        samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo compositeLayoutInfo{};
        compositeLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        compositeLayoutInfo.bindingCount = 1;
        compositeLayoutInfo.pBindings = &samplerBinding;
        vkCreateDescriptorSetLayout(context_.device, &compositeLayoutInfo, nullptr, &compositeLayout_);

        dsAlloc.pSetLayouts = &compositeLayout_;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &compositeDescriptorSet_);

        writeCompositeDescriptor();
    }

    void writeCompositeDescriptor() {
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = idSampler_;
        imageInfo.imageView = idImageView_;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = compositeDescriptorSet_;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &imageInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
    }

    void rebuildCompositePipelineDescriptor() {
        VkDescriptorSetAllocateInfo dsAlloc{};
        dsAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        dsAlloc.descriptorPool = context_.descriptorPool;
        dsAlloc.descriptorSetCount = 1;
        dsAlloc.pSetLayouts = &compositeLayout_;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &compositeDescriptorSet_);
        writeCompositeDescriptor();

        dsAlloc.pSetLayouts = &idUBOLayout_;
        vkAllocateDescriptorSets(context_.device, &dsAlloc, &idUBODescriptorSet_);

        constexpr VkDeviceSize uboSize = sizeof(glm::mat4) * 2;
        idUBOBuffer_ = createUniformBuffer(context_.device, context_.physicalDevice, uboSize);

        VkDescriptorBufferInfo bufInfo{};
        bufInfo.buffer = idUBOBuffer_.buffer;
        bufInfo.offset = 0;
        bufInfo.range = uboSize;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = idUBODescriptorSet_;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo = &bufInfo;
        vkUpdateDescriptorSets(context_.device, 1, &write, 0, nullptr);
    }

    void createIdPipeline() {
        const Shader* shader = assetManager_.get<Shader>(OBJECT_ID_SHADER);
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

        idPipeline_ = createGraphicsPipeline(
                context_.device, *shader, vertexInputState, {idUBOLayout_}, VK_FORMAT_R32_UINT, VK_FORMAT_D32_SFLOAT);
    }

    void createCompositePipeline() {
        const Shader* shader = assetManager_.get<Shader>(OUTLINE_SHADER);
        if (!shader) return;

        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        compositePipeline_ = createGraphicsPipeline(context_.device,
                                                    *shader,
                                                    vertexInputState,
                                                    {compositeLayout_},
                                                    swapchainImageFormat_,
                                                    VK_FORMAT_UNDEFINED);
    }

    void destroyIdImage() {
        if (idSampler_ != VK_NULL_HANDLE) vkDestroySampler(context_.device, idSampler_, nullptr);
        if (idImageView_ != VK_NULL_HANDLE) vkDestroyImageView(context_.device, idImageView_, nullptr);
        if (idImage_ != VK_NULL_HANDLE) vkDestroyImage(context_.device, idImage_, nullptr);
        if (idMemory_ != VK_NULL_HANDLE) vkFreeMemory(context_.device, idMemory_, nullptr);
        if (idDepthImageView_ != VK_NULL_HANDLE) vkDestroyImageView(context_.device, idDepthImageView_, nullptr);
        if (idDepthImage_ != VK_NULL_HANDLE) vkDestroyImage(context_.device, idDepthImage_, nullptr);
        if (idDepthMemory_ != VK_NULL_HANDLE) vkFreeMemory(context_.device, idDepthMemory_, nullptr);
        destroyBuffer(context_.device, idUBOBuffer_);
        idSampler_ = VK_NULL_HANDLE;
        idImageView_ = VK_NULL_HANDLE;
        idImage_ = VK_NULL_HANDLE;
        idMemory_ = VK_NULL_HANDLE;
        idDepthImageView_ = VK_NULL_HANDLE;
        idDepthImage_ = VK_NULL_HANDLE;
        idDepthMemory_ = VK_NULL_HANDLE;
        idUBODescriptorSet_ = VK_NULL_HANDLE;
        compositeDescriptorSet_ = VK_NULL_HANDLE;
    }

    void destroyResources() {
        destroyIdImage();
        if (idUBOLayout_ != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(context_.device, idUBOLayout_, nullptr);
        if (compositeLayout_ != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(context_.device, compositeLayout_, nullptr);
        if (idPipeline_.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(context_.device, idPipeline_.pipeline, nullptr);
        if (idPipeline_.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(context_.device, idPipeline_.layout, nullptr);
        if (compositePipeline_.pipeline != VK_NULL_HANDLE)
            vkDestroyPipeline(context_.device, compositePipeline_.pipeline, nullptr);
        if (compositePipeline_.layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout(context_.device, compositePipeline_.layout, nullptr);
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
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;

    // ID pass resources
    VkImage idImage_ = VK_NULL_HANDLE;
    VkDeviceMemory idMemory_ = VK_NULL_HANDLE;
    VkImageView idImageView_ = VK_NULL_HANDLE;
    VkImage idDepthImage_ = VK_NULL_HANDLE;
    VkDeviceMemory idDepthMemory_ = VK_NULL_HANDLE;
    VkImageView idDepthImageView_ = VK_NULL_HANDLE;
    VkSampler idSampler_ = VK_NULL_HANDLE;

    VulkanBuffer idUBOBuffer_{};
    VkDescriptorSetLayout idUBOLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet idUBODescriptorSet_ = VK_NULL_HANDLE;

    VkDescriptorSetLayout compositeLayout_ = VK_NULL_HANDLE;
    VkDescriptorSet compositeDescriptorSet_ = VK_NULL_HANDLE;

    VulkanPipeline idPipeline_{};
    VulkanPipeline compositePipeline_{};

    std::vector<DrawCall> outlineQueue_;
};
