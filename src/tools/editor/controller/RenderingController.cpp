#include "RenderingController.hpp"
#include <glm/glm.hpp>
#include "../../../engine/data/assets/Mesh.hpp"
#include "../../../engine/data/components/Renderer.hpp"
#include "../../../engine/data/components/Transform.hpp"
#include "../../../engine/data/settings/RendererSettings.hpp"
#include "../../../engine/systems/assets/AssetManager.hpp"
#include "../../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../../engine/systems/scene/Scene.hpp"
#include "../../../engine/utils/math/AABB.hpp"
#include "../../../engine/utils/math/BVH.hpp"
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

void RenderingController::drawGizmoLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) {
    auto& gizmoLines = editorState_.getGizmoLines();
    if (gizmoLines.size() >= VulkanGizmoPass::MAX_LINES * 2) return;
    gizmoLines.push_back({from, color});
    gizmoLines.push_back({to, color});
}

void RenderingController::drawGizmoAABB(const AABB& aabb, const glm::vec3& color) {
    const glm::vec3 min = aabb.min;
    const glm::vec3 max = aabb.max;

    // Bottom vertices
    glm::vec3 v000 = {min.x, min.y, min.z};
    glm::vec3 v001 = {min.x, min.y, max.z};
    glm::vec3 v010 = {max.x, min.y, min.z};
    glm::vec3 v011 = {max.x, min.y, max.z};

    // Top vertices
    glm::vec3 v100 = {min.x, max.y, min.z};
    glm::vec3 v101 = {min.x, max.y, max.z};
    glm::vec3 v110 = {max.x, max.y, min.z};
    glm::vec3 v111 = {max.x, max.y, max.z};

    // Bottom face (y = min.y)
    drawGizmoLine(v000, v010, color);
    drawGizmoLine(v010, v011, color);
    drawGizmoLine(v011, v001, color);
    drawGizmoLine(v001, v000, color);

    // Top face (y = max.y)
    drawGizmoLine(v100, v110, color);
    drawGizmoLine(v110, v111, color);
    drawGizmoLine(v111, v101, color);
    drawGizmoLine(v101, v100, color);

    // Vertical edges
    drawGizmoLine(v000, v100, color);
    drawGizmoLine(v001, v101, color);
    drawGizmoLine(v010, v110, color);
    drawGizmoLine(v011, v111, color);
}

void RenderingController::drawGizmoObjectAABB(const std::string& objectId, const glm::vec3& color) {
    auto& scene = editorState_.getScene();
    GameObject& object = scene.getObject(objectId);
    if (object.has<Transform>() && object.has<Renderer>()) {
        auto transform = object.get<Transform>();
        auto renderer = object.get<Renderer>();
        auto mesh = assetManager_.get<Mesh>(renderer.meshName);
        auto aabb = mesh->getAABB().applyTransform(transform.getModelMatrix());
        drawGizmoAABB(aabb, color);
    }
}

void RenderingController::drawGizmoObjectTransform(const std::string& objectId, float axisLength) {
    auto& scene = editorState_.getScene();
    GameObject& object = scene.getObject(objectId);
    if (object.has<Transform>() && object.has<Renderer>()) {
        auto transform = object.get<Transform>();
        glm::vec3 position = transform.position;
        glm::vec3 right = transform.getRight() * axisLength;
        glm::vec3 up = transform.getUp() * axisLength;
        glm::vec3 forward = transform.getForward() * axisLength;
        drawGizmoLine(position, position + right, {1.0f, 0.0f, 0.0f});
        drawGizmoLine(position, position + up, {0.0f, 1.0f, 0.0f});
        drawGizmoLine(position, position + forward, {0.0f, 0.0f, 1.0f});
    }
}

void RenderingController::drawGizmoBVH(const glm::vec3& color) {
    auto& scene = editorState_.getScene();
    std::vector<Item> items;
    for (const auto& [id, object]: scene.getObjects()) {
        if (object.has<Transform>() && object.has<Renderer>()) {
            auto transform = object.get<Transform>();
            auto renderer = object.get<Renderer>();
            auto* mesh = assetManager_.get<Mesh>(renderer.meshName);
            if (!mesh) continue;
            auto aabb = mesh->getAABB().applyTransform(transform.getModelMatrix());
            items.push_back({aabb, id});
        }
    }
    if (items.empty()) return;
    BVH bvh;
    bvh.build(std::move(items));
    for (const auto& node: bvh.nodes) {
        drawGizmoAABB(node.bounds, color);
        if (node.isLeaf()) {
            for (uint32_t i = node.begin; i < node.begin + node.count; i++)
                drawGizmoAABB(bvh.items[i].aabb, color);
        }
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
