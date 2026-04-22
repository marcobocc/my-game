#include "VulkanGraphicsBackend.hpp"
#include "VulkanRenderingOrchestrator.hpp"

VulkanGraphicsBackend::VulkanGraphicsBackend(VulkanContext& context, VulkanRenderingOrchestrator& renderer) :
    context_(context),
    renderer_(renderer) {}

VulkanGraphicsBackend::~VulkanGraphicsBackend() = default;

void VulkanGraphicsBackend::draw(const Mesh& mesh, const Material& material, const Transform& transform) const {
    renderer_.enqueueForDrawing(mesh, material, transform);
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera) const { renderer_.renderFrame(camera); }
