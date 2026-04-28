#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <volk.h>
#include "assets/AssetManager.hpp"
#include "assets/BuiltinAssetNames.hpp"
#include "assets/types/Shader.hpp"
#include "core/GameWindow.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/objects/components/Transform.hpp"
#include "rendering/vulkan/core/buffers.hpp"
#include "rendering/vulkan/core/structs.hpp"
#include "rendering/vulkan/resources/VulkanResourcesManager.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"

class VulkanGizmoPass {
public:
    static constexpr uint32_t MAX_LINES = 4096;

    VulkanGizmoPass(const VulkanContext& context,
                    AssetManager& assetManager,
                    VulkanResourcesManager& resourcesManager,
                    VulkanSwapchainManager& swapchainManager) :
        context_(context),
        assetManager_(assetManager),
        resourcesManager_(resourcesManager),
        swapchainManager_(swapchainManager) {
        initPipeline();
        initVertexBuffers();
    }

    ~VulkanGizmoPass() {
        for (auto& buf: vertexBuffers_)
            destroyBuffer(context_.device, buf);
    }

    void submitLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) {
        if (lineVertices_.size() >= MAX_LINES * 2) return;
        lineVertices_.push_back({from, color});
        lineVertices_.push_back({to, color});
    }

    void record(VkCommandBuffer cmd,
                uint32_t imageIndex,
                const Camera& camera,
                const Transform& cameraTransform,
                const GameWindow& window,
                VkImageView depthView) {
        if (pipeline_ == nullptr || lineVertices_.empty()) return;

        const VulkanSwapchain& swapchain = swapchainManager_.swapchain();
        const SceneViewport sv = window.getSceneViewport();
        const auto [scaleX, scaleY] = window.getContentScale();
        const int fbX = static_cast<int>(static_cast<float>(sv.x) * scaleX);
        const int fbY = static_cast<int>(static_cast<float>(sv.y) * scaleY);
        const int fbW = static_cast<int>(static_cast<float>(sv.width) * scaleX);
        const int fbH = static_cast<int>(static_cast<float>(sv.height) * scaleY);

        const uint32_t vertexCount = static_cast<uint32_t>(lineVertices_.size());
        const VkDeviceSize uploadSize = vertexCount * sizeof(GizmoVertex);
        updateBuffer(context_.device, vertexBuffers_[imageIndex], lineVertices_.data(), uploadSize);

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
        VkBuffer vbuf = vertexBuffers_[imageIndex].buffer;
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

        lineVertices_.clear();
    }

private:
    struct GizmoVertex {
        glm::vec3 position;
        glm::vec3 color;
    };

    void initPipeline() {
        const Shader* shader = assetManager_.get<Shader>(GIZMO_SHADER);
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
    AssetManager& assetManager_;
    VulkanResourcesManager& resourcesManager_;
    VulkanSwapchainManager& swapchainManager_;

    VulkanPipeline* pipeline_ = nullptr;
    std::vector<VulkanBuffer> vertexBuffers_;
    std::vector<GizmoVertex> lineVertices_;
};
