#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "assets/types/shader/Shader.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/shaders.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

class VulkanGridPass {
public:
    VulkanGridPass(const VulkanContext& context, AssetManager& assetManager, VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetManager_(assetManager),
        swapchainManager_(swapchainManager) {
        init();
    }

    ~VulkanGridPass() {
        if (pipeline_.pipeline != VK_NULL_HANDLE) vkDestroyPipeline(context_.device, pipeline_.pipeline, nullptr);
        if (pipeline_.layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(context_.device, pipeline_.layout, nullptr);
    }

    void
    record(VkCommandBuffer cmd, uint32_t imageIndex, const Camera& camera, const Transform& cameraTransform) const {
        if (pipeline_.pipeline == VK_NULL_HANDLE) return;

        const VulkanSwapchain& swapchain = swapchainManager_.swapchain();

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchain.swapchainImageViews[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = swapchain.depthBuffer.view;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderingInfo.renderArea = {{0, 0}, swapchain.swapchainExtent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);

        struct GridPushConstants {
            glm::mat4 invViewProj;
            glm::vec3 cameraPos;
            float pad = 0.0f;
            glm::mat4 viewProj;
        } pc;

        glm::mat4 view = cameraTransform.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix();
        pc.viewProj = proj * view;
        pc.invViewProj = glm::inverse(pc.viewProj);
        pc.cameraPos = cameraTransform.position;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_.pipeline);
        vkCmdPushConstants(cmd,
                           pipeline_.layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(GridPushConstants),
                           &pc);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

private:
    void init() {
        const Shader* shader = assetManager_.get<Shader>(GRID_SHADER);
        if (!shader) return;

        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        pipeline_ = createGraphicsPipeline(
                context_.device, *shader, vertexInputState, {}, VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT);
    }

    const VulkanContext& context_;
    AssetManager& assetManager_;
    VulkanSwapchainManager& swapchainManager_;
    VulkanPipeline pipeline_{};
};
