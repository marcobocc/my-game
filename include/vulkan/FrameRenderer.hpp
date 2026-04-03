#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

struct DrawCall {
    const MeshComponent* mesh;
    const MaterialComponent* material;
    glm::mat4 modelMatrix;
};

class FrameRenderer {
public:
    FrameRenderer(const FrameRenderer&) = delete;
    FrameRenderer& operator=(const FrameRenderer&) = delete;
    FrameRenderer(FrameRenderer&&) = delete;
    FrameRenderer& operator=(FrameRenderer&&) = delete;

    ~FrameRenderer() = default;
    FrameRenderer(VulkanPipelinesManager& pipelinesManager,
                  VulkanVertexBuffersManager& vertexBuffersManager,
                  VulkanSwapchainManager& swapchainManager);

    void renderFrame(VkCommandBuffer commandBuffer, uint32_t imageIndex, const std::vector<DrawCall>& drawCalls) const;

private:
    void beginRenderPass(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void setupViewportAndScissor(VkCommandBuffer cmd) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall) const;
    static void endRenderPass(VkCommandBuffer cmd);

    VulkanPipelinesManager& pipelinesManager_;
    VulkanVertexBuffersManager& vertexBuffersManager_;
    VulkanSwapchainManager& swapchainManager_;
};
