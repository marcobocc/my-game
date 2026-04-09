#include "vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>

VulkanGraphicsBackend::VulkanGraphicsBackend(GLFWwindow* window) :
    window_(window),
    debugMessenger_(vulkanContext_),
    commandManager_(vulkanContext_, VulkanRenderer::MAX_FRAMES_IN_FLIGHT),
    swapchainManager_(window_, vulkanContext_),
    renderer_(
            vulkanContext_, swapchainManager_.getImageCount(), vertexBufferCache_, pipelineCache_, swapchainManager_) {
    if (!window) throw std::runtime_error("Window pointer is null");
}

void VulkanGraphicsBackend::draw(const Mesh* mesh,
                                 const Material* material,
                                 const ShaderPipeline* shaderPipeline,
                                 const glm::mat4& modelMatrix) {
    drawQueue_.push_back({mesh, material, shaderPipeline, modelMatrix});
}

void VulkanGraphicsBackend::renderFrame(const CameraComponent& camera) {
    if (renderer_.renderFrame(currentFrame_,
                              commandManager_,
                              swapchainManager_,
                              vulkanContext_.getVkGraphicsQueue(),
                              drawQueue_,
                              camera)) {
        drawQueue_.clear();
    }
}
