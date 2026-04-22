#include "VulkanGraphicsBackend.hpp"
#include "VulkanRenderingOrchestrator.hpp"
#include "core/GameWindow.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"


VulkanGraphicsBackend::VulkanGraphicsBackend(GameWindow& window,
                                             VulkanContext& context,
                                             VulkanRenderingOrchestrator& renderer,
                                             VulkanSwapchainManager& swapchainManager) :
    window_(window),
    context_(context),
    renderer_(renderer),
    swapchainManager_(swapchainManager) {

    window_.onFramebufferResize([this](int, int, int, int) { swapchainManager_.recreate(context_); });
}

VulkanGraphicsBackend::~VulkanGraphicsBackend() = default;

void VulkanGraphicsBackend::draw(const Mesh& mesh, const Material& material, const Transform& transform) const {
    renderer_.enqueueForDrawing(mesh, material, transform);
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera) const { renderer_.renderFrame(camera); }
