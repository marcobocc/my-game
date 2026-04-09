#pragma once

#include <GLFW/glfw3.h>
#include "core/Material.hpp"
#include "core/Mesh.hpp"
#include "core/components/CameraComponent.hpp"
#include "services/VulkanResourceCache.hpp"
#include "vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "vulkan/raii_wrappers/VulkanDebugMessenger.hpp"
#include "vulkan/raii_wrappers/VulkanDevice.hpp"
#include "vulkan/raii_wrappers/VulkanInstance.hpp"
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

    ~VulkanGraphicsBackend();
    explicit VulkanGraphicsBackend(GLFWwindow* window);

    void draw(const Mesh& mesh, const Material& material, const glm::mat4& modelMatrix);
    void renderFrame(const CameraComponent& camera);


private:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    size_t currentFrame_ = 0;
    GLFWwindow* window_ = nullptr;
    VulkanInstance instance_;
    VulkanDebugMessenger debugMessenger_;
    VulkanDevice device_;
    VulkanCommandManager commandManager_;
    VulkanSwapchain swapchainManager_;
    VulkanResourceCache<VulkanBuffer> vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline> pipelineCache_;
    VulkanRenderer renderer_;
    std::vector<DrawCall> drawQueue_;
};
