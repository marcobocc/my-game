#include "rendering/vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>

VulkanGraphicsBackend::VulkanGraphicsBackend(AssetManager& assetManager,
                                             GLFWwindow* window,
                                             UserInterface* userInterface) :
    assetManager_(assetManager),
    window_(window),
    debugMessenger_(vulkanContext_),
    commandManager_(vulkanContext_, VulkanRenderer::MAX_FRAMES_IN_FLIGHT),
    swapchainManager_(window_, vulkanContext_),
    renderer_(vulkanContext_,
              swapchainManager_.getImageCount(),
              vertexBufferCache_,
              pipelineCache_,
              swapchainManager_,
              window_,
              userInterface,
              assetManager_) {
    if (!window) throw std::runtime_error("Window pointer is null");
}

void VulkanGraphicsBackend::draw(const Mesh& mesh, const Material& material, const Transform& transform) {
    drawQueue_.push_back({mesh, material, transform});
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera) {
    if (renderer_.renderFrame(currentFrame_,
                              commandManager_,
                              swapchainManager_,
                              vulkanContext_.getVkGraphicsQueue(),
                              drawQueue_,
                              camera)) {
        drawQueue_.clear();
    }
}
