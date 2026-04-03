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
    vertexBuffersManager_(device_.getVkDevice(), device_.getVkPhysicalDevice()),
    cameraManager_(device_.getVkDevice(), device_.getVkPhysicalDevice()),
    pipelinesManager_(
            device_.getVkDevice(), swapchainManager_.getVkRenderPass(), cameraManager_.getDescriptorSetLayout()),
    renderer_(device_.getVkDevice(),
              device_.getVkPhysicalDevice(),
              swapchainManager_.getImageCount(),
              pipelinesManager_,
              vertexBuffersManager_,
              swapchainManager_) {

    if (!window) throw std::runtime_error("Window pointer is null");
}

void VulkanGraphicsBackend::draw(const MeshComponent& mesh,
                                 const MaterialComponent& material,
                                 const glm::mat4& modelMatrix) {
    drawQueue_.push_back({&mesh, &material, modelMatrix});
}

void VulkanGraphicsBackend::renderFrame(const CameraComponent& camera) {
    cameraManager_.updateCameraUBO(currentFrame_, camera);
    if (renderer_.renderFrame(currentFrame_,
                              commandManager_,
                              swapchainManager_,
                              device_.getVkGraphicsQueue(),
                              drawQueue_,
                              cameraManager_.getDescriptorSet(currentFrame_))) {
        drawQueue_.clear();
    }
}
