#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <volk.h>
#include "../../../../modules/asset_management/asset_resources/ShaderResource.hpp"
#include "../core/VulkanSwapchainManager.hpp"
#include "../core/resources/VulkanResourcesManager.hpp"
#include "../core/utils/buffers.hpp"
#include "../core/utils/structs.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/BuiltinAssetNames.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/scene/components/Camera.hpp"
#include "modules/scene/components/Transform.hpp"

class VulkanGizmoPass {
public:
    static constexpr uint32_t MAX_LINES = 4096;

    struct GizmoVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    VulkanGizmoPass(const VulkanContext& context,
                    AssetLoader& assetLoader,
                    VulkanResourcesManager& resourcesManager,
                    VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetLoader_(assetLoader),
        resourcesManager_(resourcesManager),
        swapchainManager_(swapchainManager) {
        initPipeline();
        initVertexBuffers();
    }

    ~VulkanGizmoPass() {
        for (auto& buf: vertexBuffers_)
            destroyBuffer(context_.device, buf);
    }

    void record(VkCommandBuffer cmd,
                VkImageView colorView,
                VkExtent2D extent,
                const Camera& camera,
                const Transform& cameraTransform,
                const GameWindow& window,
                VkImageView depthView,
                const std::vector<GizmoVertex>& lineVertices) {
        if (pipeline_ == nullptr || lineVertices.empty()) return;

        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

        const uint32_t bufIdx = nextBufferIndex_++ % static_cast<uint32_t>(vertexBuffers_.size());
        const uint32_t vertexCount = static_cast<uint32_t>(lineVertices.size());
        const VkDeviceSize uploadSize = vertexCount * sizeof(GizmoVertex);
        updateBuffer(context_.device, vertexBuffers_[bufIdx], lineVertices.data(), uploadSize);

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

        glm::mat4 view = cameraTransform.getViewMatrix();
        glm::mat4 proj = camera.getProjectionMatrix();
        glm::mat4 viewProj = proj * view;

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_->pipeline);
        VkBuffer vbuf = vertexBuffers_[bufIdx].buffer;
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vbuf, &offset);
        vkCmdPushConstants(cmd,
                           pipeline_->layout,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           0,
                           sizeof(glm::mat4),
                           &viewProj);
        vkCmdDraw(cmd, vertexCount, 1, 0, 0);
        vkCmdEndRendering(cmd);
    }

private:
    void initPipeline() {
        const ShaderResource* shader = assetLoader_.get<ShaderResource>(GIZMO_SHADER);
        if (!shader) return;
        const VkFormat colorFormat = swapchainManager_.swapchain().swapchainImageFormat;
        pipeline_ = &resourcesManager_.getPipeline(*shader, colorFormat, VK_FORMAT_D32_SFLOAT);
    }

    void initVertexBuffers() {
        const uint32_t frameCount = static_cast<uint32_t>(swapchainManager_.swapchain().swapchainImages.size());
        vertexBuffers_.resize(frameCount);
        const VkDeviceSize bufferSize = MAX_LINES * 2 * sizeof(GizmoVertex);
        for (auto& buf: vertexBuffers_) {
            buf = createBuffer(context_.device,
                               context_.physicalDevice,
                               bufferSize,
                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }

    const VulkanContext& context_;
    AssetLoader& assetLoader_;
    VulkanResourcesManager& resourcesManager_;
    VulkanSwapchainManager& swapchainManager_;

    VulkanPipeline* pipeline_ = nullptr;
    std::vector<VulkanBuffer> vertexBuffers_;
    uint32_t nextBufferIndex_ = 0;
};
