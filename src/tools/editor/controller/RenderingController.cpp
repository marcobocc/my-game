#include "RenderingController.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
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

void RenderingController::drawGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color) {
    float h = halfSize;
    glm::vec3 c[8] = {
            center + glm::vec3(-h, -h, -h),
            center + glm::vec3(h, -h, -h),
            center + glm::vec3(h, h, -h),
            center + glm::vec3(-h, h, -h),
            center + glm::vec3(-h, -h, h),
            center + glm::vec3(h, -h, h),
            center + glm::vec3(h, h, h),
            center + glm::vec3(-h, h, h),
    };
    // Bottom face
    drawGizmoLine(c[0], c[1], color);
    drawGizmoLine(c[1], c[2], color);
    drawGizmoLine(c[2], c[3], color);
    drawGizmoLine(c[3], c[0], color);
    // Top face
    drawGizmoLine(c[4], c[5], color);
    drawGizmoLine(c[5], c[6], color);
    drawGizmoLine(c[6], c[7], color);
    drawGizmoLine(c[7], c[4], color);
    // Verticals
    drawGizmoLine(c[0], c[4], color);
    drawGizmoLine(c[1], c[5], color);
    drawGizmoLine(c[2], c[6], color);
    drawGizmoLine(c[3], c[7], color);
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

std::vector<GizmoHandle> RenderingController::drawTranslationHandles(const std::string& objectId,
                                                                     const Camera& camera,
                                                                     const Transform& cameraTransform) {
    auto& scene = editorState_.getScene();
    GameObject& object = scene.getObject(objectId);
    if (!object.has<Transform>()) return {};

    auto transform = object.get<Transform>();
    glm::vec3 origin = transform.position;

    // Scale handle size with camera distance so they stay constant on screen
    float dist = glm::length(cameraTransform.position - origin);
    float scale = dist * 0.15f;

    // Arrow parameters
    float shaftLength = scale;
    float headLength = scale * 0.25f;
    float headSpread = scale * 0.10f;
    float pickRadius = scale * 0.18f;
    // Capsule base is offset from origin so the three handles don't share an ambiguous anchor point
    float pickOffset = scale * 0.20f;

    const glm::vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const glm::vec3 colors[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const GizmoAxis gaxes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

    std::vector<GizmoHandle> handles;
    handles.reserve(3);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = axes[i];
        glm::vec3 color = colors[i];
        glm::vec3 tip = origin + dir * shaftLength;

        // Shaft
        drawGizmoLine(origin, tip, color);

        // Arrowhead — four lines forming a cone at the tip
        glm::vec3 perp1 =
                glm::normalize(glm::cross(dir, glm::abs(dir.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0)));
        glm::vec3 perp2 = glm::cross(dir, perp1);
        glm::vec3 headBase = tip - dir * headLength;
        drawGizmoLine(headBase + perp1 * headSpread, tip, color);
        drawGizmoLine(headBase - perp1 * headSpread, tip, color);
        drawGizmoLine(headBase + perp2 * headSpread, tip, color);
        drawGizmoLine(headBase - perp2 * headSpread, tip, color);

        // Capsule starts offset from origin to avoid three-way overlap at the center
        glm::vec3 capsuleBase = origin + dir * pickOffset;
        handles.push_back({GizmoType::Translation, gaxes[i], capsuleBase, tip, pickRadius});
    }

    return handles;
}

std::vector<GizmoHandle> RenderingController::drawRotationHandles(const std::string& objectId,
                                                                  const Camera& camera,
                                                                  const Transform& cameraTransform) {
    auto& scene = editorState_.getScene();
    GameObject& object = scene.getObject(objectId);
    if (!object.has<Transform>()) return {};

    auto transform = object.get<Transform>();
    glm::vec3 origin = transform.position;

    // Scale ring radius with camera distance so handles stay constant on screen
    float dist = glm::length(cameraTransform.position - origin);
    float ringRadius = dist * 0.15f;
    float pickRadius = ringRadius * 0.18f; // match translation handle pick radius ratio

    constexpr int segments = 32;

    // For each axis the ring lives in the plane spanned by the two perpendicular vectors
    const glm::vec3 colors[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const GizmoAxis gaxes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};
    const glm::vec3 perp1s[3] = {{0, 1, 0}, {1, 0, 0}, {1, 0, 0}};
    const glm::vec3 perp2s[3] = {{0, 0, 1}, {0, 0, 1}, {0, 1, 0}};

    std::vector<GizmoHandle> handles;
    handles.reserve(3 * segments);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 color = colors[i];
        glm::vec3 p1 = perp1s[i];
        glm::vec3 p2 = perp2s[i];

        glm::vec3 prevPt{};
        for (int s = 0; s <= segments; ++s) {
            float angle = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
            glm::vec3 pt = origin + (p1 * glm::cos(angle) + p2 * glm::sin(angle)) * ringRadius;
            if (s > 0) {
                drawGizmoLine(prevPt, pt, color);
                // Each segment doubles as a thin capsule for picking
                handles.push_back({GizmoType::Rotation, gaxes[i], prevPt, pt, pickRadius});
            }
            prevPt = pt;
        }
    }

    return handles;
}

std::vector<GizmoHandle> RenderingController::drawScaleHandles(const std::string& objectId,
                                                               const Camera& camera,
                                                               const Transform& cameraTransform) {
    auto& scene = editorState_.getScene();
    GameObject& object = scene.getObject(objectId);
    if (!object.has<Transform>()) return {};

    auto transform = object.get<Transform>();
    glm::vec3 origin = transform.position;

    float dist = glm::length(cameraTransform.position - origin);
    float scale = dist * 0.15f;

    float shaftLength = scale;
    float cubeHalf = scale * 0.07f;
    float pickRadius = scale * 0.18f;
    float pickOffset = scale * 0.20f;

    const glm::vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const glm::vec3 colors[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const GizmoAxis gaxes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

    std::vector<GizmoHandle> handles;
    handles.reserve(4);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = axes[i];
        glm::vec3 color = colors[i];
        glm::vec3 tip = origin + dir * shaftLength;

        // Shaft
        drawGizmoLine(origin, tip, color);
        // Cube tip
        drawGizmoCube(tip, cubeHalf, color);

        glm::vec3 capsuleBase = origin + dir * pickOffset;
        handles.push_back({GizmoType::Scale, gaxes[i], capsuleBase, tip + dir * cubeHalf, pickRadius});
    }

    // Center cube — uniform scale
    float centerHalf = scale * 0.09f;
    drawGizmoCube(origin, centerHalf, {1, 1, 1});
    // Represent as a short vertical capsule with radius covering the cube
    handles.push_back({GizmoType::Scale,
                       GizmoAxis::All,
                       origin - glm::vec3(0, centerHalf, 0),
                       origin + glm::vec3(0, centerHalf, 0),
                       centerHalf * 2.0f});

    return handles;
}
