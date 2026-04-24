#include "VulkanGraphicsBackend.hpp"
#include <optional>
#include "VulkanRenderingOrchestrator.hpp"

VulkanGraphicsBackend::VulkanGraphicsBackend(VulkanContext& context, VulkanRenderingOrchestrator& renderer) :
    context_(context),
    renderer_(renderer) {}

VulkanGraphicsBackend::~VulkanGraphicsBackend() = default;

void VulkanGraphicsBackend::draw(const Renderer& renderer, const Transform& transform, std::string objectId) const {
    renderer_.enqueueForDrawing(renderer, transform, std::move(objectId));
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera, const Transform& cameraTransform) const {
    renderer_.renderFrame(camera, cameraTransform);
}

void VulkanGraphicsBackend::requestPick(uint32_t x, uint32_t y) const { renderer_.requestPick(x, y); }

std::optional<std::string> VulkanGraphicsBackend::getPickResult() const { return renderer_.getPickResult(); }
