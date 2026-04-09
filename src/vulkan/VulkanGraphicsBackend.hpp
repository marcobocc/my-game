#pragma once
#include <GLFW/glfw3.h>
#include "core/assets/types/Material.hpp"
#include "core/assets/types/Mesh.hpp"
#include "core/assets/types/ShaderPipeline.hpp"
#include "core/components/CameraComponent.hpp"
#include "services/VulkanResourceCache.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/raii_wrappers/VulkanContext.hpp"
#include "vulkan/raii_wrappers/VulkanDebugMessenger.hpp"
#include "vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "vulkan/services/VulkanCommandManager.hpp"
#include "vulkan/services/VulkanRenderer.hpp"
#include "vulkan/services/VulkanResourceCache.hpp"

class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    ~VulkanGraphicsBackend() = default;
    explicit VulkanGraphicsBackend(GLFWwindow* window);

    void draw(const Mesh* mesh,
              const Material* material,
              const ShaderPipeline* shaderPipeline,
              const glm::mat4& modelMatrix);
    void renderFrame(const CameraComponent& camera);


private:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    size_t currentFrame_ = 0;
    GLFWwindow* window_ = nullptr;
    VulkanContext vulkanContext_;
    VulkanDebugMessenger debugMessenger_;
    VulkanCommandManager commandManager_;
    VulkanSwapchain swapchainManager_;
    VulkanResourceCache<VulkanBuffer> vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline> pipelineCache_;
    VulkanRenderer renderer_;
    std::vector<DrawCall> drawQueue_;
};
