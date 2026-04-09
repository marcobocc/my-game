#include "vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>
#include <volk.h>

VulkanGraphicsBackend::~VulkanGraphicsBackend() {
    if (device_.getVkDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_.getVkDevice());
    }
}

VulkanGraphicsBackend::VulkanGraphicsBackend(GLFWwindow* window) :
    window_(window),
    debugMessenger_(instance_.getVkInstance()),
    device_(instance_.getVkInstance()),
    commandManager_(device_.getVkDevice(),
                    device_.getGraphicsQueueFamilyIndex(),
                    device_.getVkGraphicsQueue(),
                    VulkanRenderer::MAX_FRAMES_IN_FLIGHT),
    swapchainManager_(window_, instance_.getVkInstance(), device_.getVkPhysicalDevice(), device_.getVkDevice()),
    renderer_(device_.getVkDevice(),
              device_.getVkPhysicalDevice(),
              swapchainManager_.getImageCount(),
              vertexBufferCache_,
              pipelineCache_,
              swapchainManager_.getVkRenderPass(),
              swapchainManager_) {

    if (!window) throw std::runtime_error("Window pointer is null");
}

void VulkanGraphicsBackend::draw(const Mesh* mesh,
                                 const Material* material,
                                 const ShaderPipeline* shaderPipeline,
                                 const glm::mat4& modelMatrix) {
    drawQueue_.push_back({mesh, material, shaderPipeline, modelMatrix});
}

void VulkanGraphicsBackend::renderFrame(const CameraComponent& camera) {
    if (renderer_.renderFrame(
                currentFrame_, commandManager_, swapchainManager_, device_.getVkGraphicsQueue(), drawQueue_, camera)) {
        drawQueue_.clear();
    }
}
