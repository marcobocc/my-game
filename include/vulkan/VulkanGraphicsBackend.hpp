#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "ecs/components/MaterialComponent.hpp"
#include "ecs/components/MeshComponent.hpp"
#include "vulkan/VulkanCommandManager.hpp"
#include "vulkan/VulkanDebugMessenger.hpp"
#include "vulkan/VulkanDevice.hpp"
#include "vulkan/VulkanInstance.hpp"
#include "vulkan/VulkanPipelinesManager.hpp"
#include "vulkan/VulkanSwapchainManager.hpp"
#include "vulkan/VulkanVertexBuffersManager.hpp"

class VulkanGraphicsBackend {
public:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;
    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    ~VulkanGraphicsBackend();
    explicit VulkanGraphicsBackend(GLFWwindow* window);
    void draw(const MeshComponent& mesh, const MaterialComponent& material);
    void renderFrame();

private:
    void renderEntity(VkCommandBuffer cmd, const MeshComponent& mesh, const MaterialComponent& material);

    struct DrawCall {
        const MeshComponent* mesh;
        const MaterialComponent* material;
    };

    size_t currentFrame_ = 0;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> imageAvailableSemaphores_{};
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> renderFinishedSemaphores_{};
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> inFlightFences_{};
    GLFWwindow* window_ = nullptr; // TODO: Create window wrapper object
    VulkanInstance instance_;
    VulkanDebugMessenger debugMessenger_;
    VulkanDevice device_;
    VulkanCommandManager commandManager_;
    VulkanSwapchainManager swapchainManager_;
    VulkanPipelinesManager pipelinesManager_;
    VulkanVertexBuffersManager vertexBuffersManager_;
    std::vector<DrawCall> drawQueue_;
};
