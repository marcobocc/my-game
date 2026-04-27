#include "VulkanGraphicsBackend.hpp"
#include <glm/vec3.hpp>
#include "VulkanRenderingOrchestrator.hpp"
#include "rendering/IPickingBackend.hpp"

VulkanGraphicsBackend::VulkanGraphicsBackend(VulkanContext& context,
                                             VulkanRenderingOrchestrator& renderer,
                                             IPickingBackend& pickingBackend) :
    context_(context),
    renderer_(renderer),
    pickingBackend_(pickingBackend) {}

VulkanGraphicsBackend::~VulkanGraphicsBackend() = default;

void VulkanGraphicsBackend::draw(const Renderer& renderer, const Transform& transform, std::string objectId) const {
    renderer_.enqueueForDrawing(renderer, transform, std::move(objectId));
}

void VulkanGraphicsBackend::outline(const Renderer& renderer, const Transform& transform, std::string objectId) const {
    renderer_.enqueueForOutline(renderer, transform, std::move(objectId));
}

void VulkanGraphicsBackend::submitGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const {
    renderer_.submitGizmoLine(from, to, color);
}

void VulkanGraphicsBackend::renderFrame(const Camera& camera, const Transform& cameraTransform) const {
    renderer_.renderFrame(camera, cameraTransform);
}

IPickingBackend& VulkanGraphicsBackend::pickingBackend() const { return pickingBackend_; }
