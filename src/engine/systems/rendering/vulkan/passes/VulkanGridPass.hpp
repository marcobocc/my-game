#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/structs.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Transform.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/assets/BuiltinAssetNames.hpp"

class VulkanGridPass {
public:
    VulkanGridPass(AssetManager& assetManager, VulkanResourcesManager& resourcesManager) :
        assetManager_(assetManager),
        resourcesManager_(resourcesManager) {
        initPipeline();
    }

    ~VulkanGridPass() = default;

    void record(VkCommandBuffer cmd,
                VkImageView colorView,
                VkExtent2D extent,
                const Camera& camera,
                const Transform& cameraTransform,
                float gridScale,
                VkImageView depthView) const {
        if (pipeline_ == nullptr) return;

        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorView;
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
        renderingInfo.renderArea = {{0, 0}, extent};
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(cmd, &renderingInfo);

        struct GridPushConstants {
            glm::mat4 invViewProj;
            glm::mat4 viewProj;
            glm::vec3 cameraPos;
            float scale;
        } pc{};

        glm::mat4 view = cameraTransform.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix();
        pc.viewProj = proj * view;
        pc.invViewProj = glm::inverse(pc.viewProj);
        pc.cameraPos = cameraTransform.position;
        pc.scale = gridScale;

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
    VulkanPipeline* pipeline_ = nullptr;
};
