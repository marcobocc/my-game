#pragma once
#include <glm/glm.hpp>
#include <volk.h>
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/structs.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanGridPass {
public:
    VulkanGridPass(AssetLoader& assetLoader, VulkanResourcesManager& resourcesManager) :
        assetLoader_(assetLoader),
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
                VkImageView depthView,
                const GameWindow& window) const {
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

        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const float fbX = static_cast<float>(sv.x) * scaleX;
        const float fbY = static_cast<float>(sv.y) * scaleY;
        const float fbW = static_cast<float>(sv.width) * scaleX;
        const float fbH = static_cast<float>(sv.height) * scaleY;

        VkViewport viewport{fbX, fbY, fbW, fbH, 0.0f, 1.0f};
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        VkRect2D scissor{{static_cast<int32_t>(fbX), static_cast<int32_t>(fbY)},
                         {static_cast<uint32_t>(fbW), static_cast<uint32_t>(fbH)}};
        vkCmdSetScissor(cmd, 0, 1, &scissor);

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
        const Shader* shader = assetLoader_.get<Shader>(GRID_SHADER);
        if (!shader) return;
        pipeline_ = &resourcesManager_.getPipeline(*shader);
    }

    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;
    VulkanPipeline* pipeline_ = nullptr;
};
