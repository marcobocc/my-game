#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

class VulkanGridPass {
public:
    VulkanGridPass(AssetManager& assetManager,
                   VulkanResourcesManager& resourcesManager,
                   VulkanSwapchainManager& swapchainManager) :
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        swapchainManager_(swapchainManager) {
        initPipeline();
    }

    ~VulkanGridPass() = default;

    void record(VkCommandBuffer cmd,
                uint32_t imageIndex,
                const Camera& camera,
                const Transform& cameraTransform,
                VkImageView depthView) const {
        if (pipeline_ == nullptr) return;

        const VulkanSwapchain& swapchain = swapchainManager_.swapchain();

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = swapchain.swapchainImageViews[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthView;
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
        } pc{};

        glm::mat4 view = cameraTransform.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix();
        pc.viewProj = proj * view;
        pc.invViewProj = glm::inverse(pc.viewProj);
        pc.cameraPos = cameraTransform.position;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(GridPushConstants),
                           &pc);
        vkCmdDraw(cmd, 3, 1, 0, 0);

        vkCmdEndRendering(cmd);
    }

private:
    void initPipeline() {
        const Shader* shader = assetManager_.get<Shader>(GRID_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader);
    }

    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;
    VulkanSwapchainManager& swapchainManager_;
    VulkanPipeline* pipeline_ = nullptr; // owned by VulkanPipelineCache
};
