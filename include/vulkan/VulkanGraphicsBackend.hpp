#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "vulkan/VulkanFramesManager.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDebugMessenger.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    ~VulkanGraphicsBackend();
    explicit VulkanGraphicsBackend(GLFWwindow* window);

    void draw(const MeshComponent& mesh, const MaterialComponent& material, const glm::mat4& modelMatrix);
    void renderFrame();

private:
    size_t currentFrame_ = 0;
    GLFWwindow* window_ = nullptr;
    VulkanInstance instance_;
    VulkanDebugMessenger debugMessenger_;
    VulkanDevice device_;
    VulkanCommandManager commandManager_;
    VulkanSwapchainManager swapchainManager_;
    VulkanPipelinesManager pipelinesManager_;
    VulkanVertexBuffersManager vertexBuffersManager_;
    VulkanFramesManager framesManager_;
    std::vector<DrawCall> drawQueue_;
};
