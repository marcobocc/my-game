#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include "core/assets/types/Material.hpp"
#include "core/assets/types/Mesh.hpp"
#include "core/assets/types/ShaderPipeline.hpp"
#include "core/components/CameraComponent.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/raii_wrappers/VulkanPipeline.hpp"
#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "vulkan/raii_wrappers/VulkanUBO.hpp"
#include "vulkan/services/VulkanResourceCache.hpp"

struct DrawCall {
    const Mesh* mesh;
    const Material* material;
    const ShaderPipeline* shaderPipeline;
    const glm::mat4 modelMatrix;
};

class VulkanSceneRenderer {
public:
    VulkanSceneRenderer(VkDevice device,
                        VkPhysicalDevice physicalDevice,
                        VulkanResourceCache<VulkanBuffer>& vertexBufferCache,
                        VulkanResourceCache<VulkanPipeline>& pipelineCache,
                        VulkanSwapchain& swapchain,
                        VulkanUBO& cameraUBO);

    void recordScenePass(VkCommandBuffer cmd,
                         uint32_t imageIndex,
                         size_t currentFrame,
                         const std::vector<DrawCall>& drawCalls);

private:
    void beginRendering(VkCommandBuffer cmd, uint32_t imageIndex) const;
    void endRendering(VkCommandBuffer cmd) const;
    void setupViewportAndScissor(VkCommandBuffer cmd) const;
    void renderEntity(VkCommandBuffer cmd, const DrawCall& drawCall, size_t currentFrame);

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VulkanResourceCache<VulkanBuffer>& vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline>& pipelineCache_;
    VulkanSwapchain& swapchain_;
    VulkanUBO& cameraUBO_;
};
