#include "VulkanGraphicsBackend.hpp"
#include "VulkanRenderingOrchestrator.hpp"
#include "rendering/vulkan/services/VulkanSwapchainManager.hpp"


VulkanGraphicsBackend::VulkanGraphicsBackend(VulkanContext& context,
                                             VulkanRenderingOrchestrator& renderer,
                                             VulkanSwapchainManager& swapchainManager) :
    context_(context),
    renderer_(renderer),
    swapchainManager_(swapchainManager) {}

VulkanGraphicsBackend::~VulkanGraphicsBackend() = default;

void VulkanGraphicsBackend::draw(const Mesh& mesh, const Material& material, const Transform& transform) const {
    renderer_.enqueueForDrawing(mesh, material, transform);
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera) const { renderer_.renderFrame(camera); }

void VulkanGraphicsBackend::recreateSwapchain() const { swapchainManager_.recreate(context_); }
