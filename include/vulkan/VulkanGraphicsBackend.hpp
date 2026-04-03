#pragma once

#include <GLFW/glfw3.h>
#include "ecs/components/CameraComponent.hpp"
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "vulkan/VulkanCameraManager.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDebugMessenger.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanFramesManager.hpp"
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
    void renderFrame(const CameraComponent& camera);

private:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    size_t currentFrame_ = 0;
    GLFWwindow* window_ = nullptr;
    VulkanInstance instance_;
    VulkanDebugMessenger debugMessenger_;
    VulkanDevice device_;
    VulkanCommandManager commandManager_;
    VulkanSwapchainManager swapchainManager_;
    VulkanVertexBuffersManager vertexBuffersManager_;
    VulkanCameraManager cameraManager_;
    VulkanPipelinesManager pipelinesManager_;
    VulkanFramesManager framesManager_;
    std::vector<DrawCall> drawQueue_;
};
