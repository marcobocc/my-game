#pragma once
#include <GLFW/glfw3.h>
#include "core/objects/components/Material.hpp"
#include "core/assets/types/MeshData.hpp"
#include "core/assets/types/ShaderPipeline.hpp"
#include "core/objects/components/Camera.hpp"
#include "core/ui/UserInterface.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanBuffer.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanContext.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanDebugMessenger.hpp"
#include "rendering/vulkan/raii_wrappers/VulkanSwapchain.hpp"
#include "rendering/vulkan/services/VulkanCommandManager.hpp"
#include "rendering/vulkan/services/VulkanRenderer.hpp"
#include "services/VulkanResourceCache.hpp"

class VulkanGraphicsBackend {
public:
    VulkanGraphicsBackend(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend& operator=(const VulkanGraphicsBackend&) = delete;
    VulkanGraphicsBackend(VulkanGraphicsBackend&&) = delete;
    VulkanGraphicsBackend& operator=(VulkanGraphicsBackend&&) = delete;

    ~VulkanGraphicsBackend() = default;
    explicit VulkanGraphicsBackend(GLFWwindow* window, UserInterface* userInterface);

    void draw(const MeshData* mesh,
              const Material* material,
              const ShaderPipeline* shaderPipeline,
              const glm::mat4& modelMatrix);

    void renderFrame(const Camera& camera);


private:
    static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

    size_t currentFrame_ = 0;
    GLFWwindow* window_ = nullptr;
    VulkanContext vulkanContext_{};
    VulkanDebugMessenger debugMessenger_;
    VulkanCommandManager commandManager_;
    VulkanSwapchain swapchainManager_;
    VulkanResourceCache<VulkanBuffer> vertexBufferCache_;
    VulkanResourceCache<VulkanPipeline> pipelineCache_;
    VulkanRenderer renderer_;
    std::vector<DrawCall> drawQueue_;
};
