#include "rendering/vulkan/VulkanGraphicsBackend.hpp"
#include <stdexcept>
#include <volk.h>

VulkanGraphicsBackend::VulkanGraphicsBackend(AssetManager& assetManager,
                                             GLFWwindow* window,
                                             UserInterface* userInterface) :
    assetManager_(assetManager),
    window_(window),
    debugMessenger_(vulkanContext_),
    commandManager_(vulkanContext_, VulkanRenderer::MAX_FRAMES_IN_FLIGHT),
    swapchainManager_(window_, vulkanContext_),
    textureSet_(vulkanContext_.getVkDevice(), vulkanContext_.getVkDescriptorPool()),
    renderer_(std::make_unique<VulkanRenderer>(vulkanContext_,
                                               swapchainManager_.getImageCount(),
                                               vertexBufferCache_,
                                               pipelineCache_,
                                               textureCache_,
                                               swapchainManager_,
                                               window_,
                                               userInterface,
                                               &textureSet_,
                                               assetManager)),
    userInterface_(userInterface) {
    if (!window) throw std::runtime_error("Window pointer is null");
}

VulkanGraphicsBackend::~VulkanGraphicsBackend() {
    renderer_.reset();
}

void VulkanGraphicsBackend::draw(const Mesh& mesh, const Material& material, const Transform& transform) {
    drawQueue_.push_back({mesh, material, transform});
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera) {
    if (renderer_->renderFrame(currentFrame_,
                               commandManager_,
                               swapchainManager_,
                               vulkanContext_.getVkGraphicsQueue(),
                               drawQueue_,
                               camera)) {
        drawQueue_.clear();
    }
}

void VulkanGraphicsBackend::recreateSwapchain() {
    vkDeviceWaitIdle(vulkanContext_.getVkDevice());
    swapchainManager_.recreate();
    renderer_.reset();
    renderer_ = std::make_unique<VulkanRenderer>(vulkanContext_,
                                                 swapchainManager_.getImageCount(),
                                                 vertexBufferCache_,
                                                 pipelineCache_,
                                                 textureCache_,
                                                 swapchainManager_,
                                                 window_,
                                                 userInterface_,
                                                 &textureSet_,
                                                 assetManager_);
}
