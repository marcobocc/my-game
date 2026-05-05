#include "RenderingController.hpp"
#include "../../../engine/data/components/Renderer.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/data/settings/RendererSettings.hpp"
#include "../../../engine/systems/assets/AssetManager.hpp"
#include "../../../engine/systems/scene/Scene.hpp"
#include "../rendering/EditorRenderSystem.hpp"
#include "../rendering/VulkanEditorRenderer.hpp"

RenderingController::RenderingController(EditorState& state,
                                         VulkanEditorRenderer& renderer,
                                         EditorRenderSystem& renderSystem,
                                         RendererSettings& settings,
                                         AssetManager& assetManager) :
    editorState_(state),
    renderer_(renderer),
    renderSystem_(renderSystem),
    settings_(settings),
    assetManager_(assetManager) {}

void RenderingController::enableWorldGrid() {
    editorState_.setGridEnabled(true);
    settings_.enableGrid = true;
}

void RenderingController::disableWorldGrid() {
    editorState_.setGridEnabled(false);
    settings_.enableGrid = false;
}

void RenderingController::toggleWorldGrid() {
    bool enabled = !editorState_.isGridEnabled();
    editorState_.setGridEnabled(enabled);
    settings_.enableGrid = enabled;
}

void RenderingController::enableLighting() {
    editorState_.setLightingEnabled(true);
    settings_.enableLighting = true;
}

void RenderingController::disableLighting() {
    editorState_.setLightingEnabled(false);
    settings_.enableLighting = false;
}

void RenderingController::toggleLighting() {
    bool enabled = !editorState_.isLightingEnabled();
    editorState_.setLightingEnabled(enabled);
    settings_.enableLighting = enabled;
}

void RenderingController::setActiveCamera(const Camera& camera, const Transform& cameraTransform) {
    editorState_.setActiveCamera(camera, cameraTransform);
    renderSystem_.setActiveCamera(camera, cameraTransform);
}

void RenderingController::drawObjectOutline(const std::string& objectId) {
    auto& scene = editorState_.getScene();
    GameObject& object = scene.getObject(objectId);
    if (object.has<Renderer>() && object.has<Transform>()) {
        auto renderer = object.get<Renderer>();
        auto transform = object.get<Transform>();
        editorState_.getOutlineQueue().push_back({renderer, transform, objectId});
    }
}

RenderTargetHandle RenderingController::createRenderTarget(uint32_t width, uint32_t height) {
    return renderer_.createRenderTarget(width, height);
}

void RenderingController::destroyRenderTarget(RenderTargetHandle handle) { renderer_.destroyRenderTarget(handle); }

VkDescriptorSet RenderingController::getRenderTargetImGuiId(RenderTargetHandle handle) const {
    return editorState_.getRenderTargetImGuiId(handle);
}

void RenderingController::renderToTarget(RenderTargetHandle handle,
                                         const Camera& camera,
                                         const Transform& cameraTransform) {
    if (!handle.isValid()) return;
    Camera offscreenCamera = camera;
    offscreenCamera.renderTarget = handle;
    renderer_.queueOffscreenCamera(offscreenCamera, cameraTransform);
}
