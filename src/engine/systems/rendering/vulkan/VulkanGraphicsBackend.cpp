#include "VulkanGraphicsBackend.hpp"
#include <glm/vec3.hpp>
#include "VulkanRenderingOrchestrator.hpp"
#include "systems/rendering/IPickingBackend.hpp"

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

RenderTargetHandle VulkanGraphicsBackend::createRenderTarget(uint32_t width, uint32_t height) const {
    return renderer_.createRenderTarget(width, height);
}

void VulkanGraphicsBackend::destroyRenderTarget(RenderTargetHandle handle) const {
    renderer_.destroyRenderTarget(handle);
}

VkDescriptorSet VulkanGraphicsBackend::getRenderTargetImGuiId(RenderTargetHandle handle) const {
    return renderer_.getRenderTargetImGuiId(handle);
}

void VulkanGraphicsBackend::renderToTarget(RenderTargetHandle handle,
                                           const Camera& camera,
                                           const Transform& cameraTransform,
                                           const std::vector<DrawCall>& drawQueue) const {
    renderer_.renderToTarget(handle, camera, cameraTransform, drawQueue);
}

void VulkanGraphicsBackend::renderToTarget(RenderTargetHandle handle,
                                           const Camera& camera,
                                           const Transform& cameraTransform) const {
    renderer_.renderToTarget(handle, camera, cameraTransform);
}
