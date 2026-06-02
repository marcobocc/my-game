#include "ObjectTransformHandle.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/EditorSettings.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../editor_camera/EditorCamera.hpp"
#include "modules/input/RaycastPickingSystem.hpp"
#include "modules/scene/components/Transform.hpp"

std::optional<glm::vec3>
ObjectTransformHandle::rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint, const glm::vec3& planeNormal) {
    float denom = glm::dot(ray.direction, planeNormal);
    if (std::abs(denom) < 1e-5f) return std::nullopt;
    float t = glm::dot(planePoint - ray.origin, planeNormal) / denom;
    if (t < 0.0f) return std::nullopt;
    return ray.origin + ray.direction * t;
}

float ObjectTransformHandle::projectOnAxis(const glm::vec3& point,
                                           const glm::vec3& axisOrigin,
                                           const glm::vec3& axisDir) {
    return glm::dot(point - axisOrigin, axisDir);
}

Ray ObjectTransformHandle::buildMouseRay(double mouseX, double mouseY) const {
    auto sv = window_.getSceneViewport();
    float ndcX = (static_cast<float>(mouseX) - static_cast<float>(sv.x)) / static_cast<float>(sv.width);
    float ndcY = (static_cast<float>(mouseY) - static_cast<float>(sv.y)) / static_cast<float>(sv.height);
    return RaycastPickingSystem::buildRay(
            {ndcX, ndcY}, editorOrbitCamera_.getCamera(), editorOrbitCamera_.getCameraTransform());
}

glm::vec3 ObjectTransformHandle::axisToDir(GizmoAxis axis) {
    switch (axis) {
        case GizmoAxis::X:
            return {1, 0, 0};
        case GizmoAxis::Y:
            return {0, 1, 0};
        case GizmoAxis::Z:
            return {0, 0, 1};
        case GizmoAxis::All:
            return {1, 1, 1};
    }
    return {1, 0, 0};
}

glm::vec3 ObjectTransformHandle::transformAxisToLocalSpace(GizmoAxis axis, const Transform& transform) {
    if (!rendererSettings_.isLocalTransformEnabled()) {
        return axisToDir(axis);
    }
    glm::vec3 worldAxis = axisToDir(axis);
    glm::mat3 rotationMatrix = glm::mat3_cast(transform.rotation);
    return glm::normalize(rotationMatrix * worldAxis);
}

void ObjectTransformHandle::beginTranslationDrag(GizmoAxis axis, double mouseX, double mouseY) {
    const auto& ids = editorSelection_.getSelectedEntityIds();
    if (ids.size() != 1) return;
    EntityHandle selectedId = ids[0];

    const auto* transformPtr = scene_.getObject(selectedId).getComponent<Transform>();
    if (!transformPtr) return;

    glm::vec3 axisDir = transformAxisToLocalSpace(axis, *transformPtr);
    glm::vec3 origin = transformPtr->position;

    glm::vec3 camDir = glm::normalize(editorOrbitCamera_.getCameraTransform().position - origin);
    glm::vec3 planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
    if (glm::length(planeNormal) < 1e-5f) planeNormal = editorOrbitCamera_.getCameraTransform().getUp();

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, origin, planeNormal);
    if (!hitPoint) return;

    dragGroupId_ = UndoHistory::randomGroupId("Translate");
    dragState_.activeDrag.emplace(selectedId, axis, axisDir, planeNormal, origin, *hitPoint);
}

void ObjectTransformHandle::updateTranslationDrag(double mouseX, double mouseY) {
    if (!dragState_.activeDrag) return;
    const auto* transformPtr = scene_.getObject(dragState_.activeDrag->objectId).getComponent<Transform>();
    if (!transformPtr) return;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, dragState_.activeDrag->dragOrigin, dragState_.activeDrag->dragPlaneNormal);
    if (!hitPoint) return;

    float startT = projectOnAxis(
            dragState_.activeDrag->hitPointOnAxis, dragState_.activeDrag->dragOrigin, dragState_.activeDrag->axisDir);
    float currentT = projectOnAxis(*hitPoint, dragState_.activeDrag->dragOrigin, dragState_.activeDrag->axisDir);
    float delta = currentT - startT;
    glm::vec3 pos = dragState_.activeDrag->dragOrigin + dragState_.activeDrag->axisDir * delta;

    if (rendererSettings_.isGridSnappingEnabled()) {
        float scale = rendererSettings_.getGridScale();
        glm::vec3 gridOrigin = {0.0f, 0.0f, 0.0f};
        pos = gridOrigin + glm::round((pos - gridOrigin) / scale) * scale;
    }

    scene_.getObject(dragState_.activeDrag->objectId)
            .mutateComponent<Transform>([pos](Transform& t) { t.position = pos; }, dragGroupId_);
}

void ObjectTransformHandle::commitTranslationDrag() {
    dragState_.activeDrag.reset();
    dragGroupId_.clear();
}

void ObjectTransformHandle::beginRotationDrag(GizmoAxis axis, double mouseX, double mouseY) {
    const auto& ids = editorSelection_.getSelectedEntityIds();
    if (ids.size() != 1) return;
    EntityHandle selectedId = ids[0];

    const auto* transformPtr = scene_.getObject(selectedId).getComponent<Transform>();
    if (!transformPtr) return;

    glm::vec3 axisDir = transformAxisToLocalSpace(axis, *transformPtr);
    glm::vec3 origin = transformPtr->position;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, origin, axisDir);
    if (!hitPoint) return;

    glm::vec3 hitDir = *hitPoint - origin;
    if (glm::length(hitDir) < 1e-5f) return;
    hitDir = glm::normalize(hitDir);

    dragGroupId_ = UndoHistory::randomGroupId("Rotate");
    dragState_.activeRotationDrag.emplace(selectedId, axis, axisDir, origin, transformPtr->rotation, hitDir);
}

void ObjectTransformHandle::updateRotationDrag(double mouseX, double mouseY) {
    if (!dragState_.activeRotationDrag) return;
    const auto* transformPtr = scene_.getObject(dragState_.activeRotationDrag->objectId).getComponent<Transform>();
    if (!transformPtr) return;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint =
            rayPlaneIntersect(ray, dragState_.activeRotationDrag->origin, dragState_.activeRotationDrag->axisDir);
    if (!hitPoint) return;

    glm::vec3 hitDir = *hitPoint - dragState_.activeRotationDrag->origin;
    if (glm::length(hitDir) < 1e-5f) return;
    hitDir = glm::normalize(hitDir);

    float cosA = glm::clamp(glm::dot(dragState_.activeRotationDrag->initialHitDir, hitDir), -1.0f, 1.0f);
    float angle = glm::acos(cosA);
    if (glm::dot(glm::cross(dragState_.activeRotationDrag->initialHitDir, hitDir),
                 dragState_.activeRotationDrag->axisDir) < 0.0f)
        angle = -angle;

    glm::quat newRotation = glm::angleAxis(angle, dragState_.activeRotationDrag->axisDir) *
                            dragState_.activeRotationDrag->initialRotation;

    scene_.getObject(dragState_.activeRotationDrag->objectId)
            .mutateComponent<Transform>([newRotation](Transform& t) { t.rotation = newRotation; }, dragGroupId_);
}

void ObjectTransformHandle::commitRotationDrag() {
    dragState_.activeRotationDrag.reset();
    dragGroupId_.clear();
}

void ObjectTransformHandle::beginScaleDrag(GizmoAxis axis, double mouseX, double mouseY) {
    const auto& ids = editorSelection_.getSelectedEntityIds();
    if (ids.size() != 1) return;
    EntityHandle selectedId = ids[0];
    const auto* transformPtr = scene_.getObject(selectedId).getComponent<Transform>();
    if (!transformPtr) return;

    glm::vec3 origin = transformPtr->position;

    glm::vec3 axisDir;
    glm::vec3 planeNormal;
    if (axis == GizmoAxis::All) {
        axisDir = glm::normalize(editorOrbitCamera_.getCameraTransform().getRight());
        planeNormal = glm::normalize(editorOrbitCamera_.getCameraTransform().position - origin);
    } else {
        glm::vec3 worldAxis = axisToDir(axis);
        glm::mat3 rotationMatrix = glm::mat3_cast(transformPtr->rotation);
        axisDir = glm::normalize(rotationMatrix * worldAxis);
        glm::vec3 camDir = glm::normalize(editorOrbitCamera_.getCameraTransform().position - origin);
        planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
        if (glm::length(planeNormal) < 1e-5f) planeNormal = editorOrbitCamera_.getCameraTransform().getUp();
    }

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, origin, planeNormal);
    if (!hitPoint) return;

    float initialT = glm::dot(*hitPoint - origin, axisDir);
    if (std::abs(initialT) < 1e-5f) return;

    dragGroupId_ = UndoHistory::randomGroupId("Scale");
    dragState_.activeScaleDrag.emplace(selectedId, axis, axisDir, origin, transformPtr->scale, planeNormal, initialT);
}

void ObjectTransformHandle::updateScaleDrag(double mouseX, double mouseY) {
    if (!dragState_.activeScaleDrag) return;
    const auto* transformPtr = scene_.getObject(dragState_.activeScaleDrag->objectId).getComponent<Transform>();
    if (!transformPtr) return;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint =
            rayPlaneIntersect(ray, dragState_.activeScaleDrag->origin, dragState_.activeScaleDrag->dragPlaneNormal);
    if (!hitPoint) return;

    float currentT = glm::dot(*hitPoint - dragState_.activeScaleDrag->origin, dragState_.activeScaleDrag->axisDir);
    float factor = glm::max(currentT / dragState_.activeScaleDrag->initialT, 0.01f);

    glm::vec3 newScale = dragState_.activeScaleDrag->initialScale;
    if (dragState_.activeScaleDrag->axis == GizmoAxis::All) {
        newScale = dragState_.activeScaleDrag->initialScale * factor;
    } else {
        switch (dragState_.activeScaleDrag->axis) {
            case GizmoAxis::X:
                newScale.x = dragState_.activeScaleDrag->initialScale.x * factor;
                break;
            case GizmoAxis::Y:
                newScale.y = dragState_.activeScaleDrag->initialScale.y * factor;
                break;
            case GizmoAxis::Z:
                newScale.z = dragState_.activeScaleDrag->initialScale.z * factor;
                break;
            default:
                break;
        }
    }

    scene_.getObject(dragState_.activeScaleDrag->objectId)
            .mutateComponent<Transform>([newScale](Transform& t) { t.scale = newScale; }, dragGroupId_);
}

void ObjectTransformHandle::commitScaleDrag() {
    dragState_.activeScaleDrag.reset();
    dragGroupId_.clear();
}

void ObjectTransformHandle::beginDrag(GizmoType type, GizmoAxis axis, double mouseX, double mouseY) {
    if (type == GizmoType::Translation) {
        beginTranslationDrag(axis, mouseX, mouseY);
    } else if (type == GizmoType::Rotation) {
        beginRotationDrag(axis, mouseX, mouseY);
    } else if (type == GizmoType::Scale) {
        beginScaleDrag(axis, mouseX, mouseY);
    }
}

void ObjectTransformHandle::updateDrag(double mouseX, double mouseY) {
    if (dragState_.activeDrag) {
        updateTranslationDrag(mouseX, mouseY);
    } else if (dragState_.activeRotationDrag) {
        updateRotationDrag(mouseX, mouseY);
    } else if (dragState_.activeScaleDrag) {
        updateScaleDrag(mouseX, mouseY);
    }
}

void ObjectTransformHandle::endDrag() {
    if (dragState_.activeDrag) {
        commitTranslationDrag();
    } else if (dragState_.activeRotationDrag) {
        commitRotationDrag();
    } else if (dragState_.activeScaleDrag) {
        commitScaleDrag();
    }
}

void ObjectTransformHandle::addGizmoLine(GizmoObject& gizmoObject,
                                         const glm::vec3& from,
                                         const glm::vec3& to,
                                         const glm::vec3& color) {
    if (gizmoObject.size() >= VulkanGizmoPass::MAX_LINES * 2) return;
    gizmoObject.push_back({from, color});
    gizmoObject.push_back({to, color});
}

static glm::vec3 rotateVector(const glm::vec3& vector, const glm::quat& rotation) {
    glm::mat3 rotationMatrix = glm::mat3_cast(rotation);
    return rotationMatrix * vector;
}

ObjectTransformHandle::GizmoObject
ObjectTransformHandle::buildGizmoCube(const glm::vec3& center, float halfSize, const glm::vec3& color) {
    GizmoObject gizmoObject;
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
    addGizmoLine(gizmoObject, c[0], c[1], color);
    addGizmoLine(gizmoObject, c[1], c[2], color);
    addGizmoLine(gizmoObject, c[2], c[3], color);
    addGizmoLine(gizmoObject, c[3], c[0], color);
    addGizmoLine(gizmoObject, c[4], c[5], color);
    addGizmoLine(gizmoObject, c[5], c[6], color);
    addGizmoLine(gizmoObject, c[6], c[7], color);
    addGizmoLine(gizmoObject, c[7], c[4], color);
    addGizmoLine(gizmoObject, c[0], c[4], color);
    addGizmoLine(gizmoObject, c[1], c[5], color);
    addGizmoLine(gizmoObject, c[2], c[6], color);
    addGizmoLine(gizmoObject, c[3], c[7], color);
    return gizmoObject;
}

ObjectTransformHandle::GizmoTransformHandle ObjectTransformHandle::buildTranslationGizmo(
        EntityHandle objectId, const Camera& camera, const Transform& cameraTransform) {
    GizmoTransformHandle result;
    const auto* transform = scene_.getObject(objectId).getComponent<Transform>();
    if (!transform) return result;

    bool isLocal = rendererSettings_.isLocalTransformEnabled();
    glm::vec3 origin = transform->position;

    float dist = glm::length(cameraTransform.position - origin);
    float scale = dist * 0.15f;

    float shaftLength = scale;
    float headLength = scale * 0.25f;
    float headSpread = scale * 0.10f;
    float pickRadius = scale * 0.18f;
    float pickOffset = scale * 0.20f;

    const glm::vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const glm::vec3 colors[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const GizmoAxis gaxes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

    result.pickingHandles.reserve(3);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = axes[i];
        if (isLocal) dir = rotateVector(dir, transform->rotation);
        glm::vec3 color = colors[i];
        glm::vec3 tip = origin + dir * shaftLength;

        addGizmoLine(result.visualization, origin, tip, color);

        glm::vec3 perp1 =
                glm::normalize(glm::cross(dir, glm::abs(dir.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0)));
        glm::vec3 perp2 = glm::cross(dir, perp1);
        glm::vec3 headBase = tip - dir * headLength;
        addGizmoLine(result.visualization, headBase + perp1 * headSpread, tip, color);
        addGizmoLine(result.visualization, headBase - perp1 * headSpread, tip, color);
        addGizmoLine(result.visualization, headBase + perp2 * headSpread, tip, color);
        addGizmoLine(result.visualization, headBase - perp2 * headSpread, tip, color);

        glm::vec3 capsuleBase = origin + dir * pickOffset;
        result.pickingHandles.emplace_back(objectId, GizmoType::Translation, gaxes[i], capsuleBase, tip, pickRadius);
    }

    return result;
}

ObjectTransformHandle::GizmoTransformHandle ObjectTransformHandle::buildRotationGizmo(
        EntityHandle objectId, const Camera& camera, const Transform& cameraTransform) {
    GizmoTransformHandle result;
    const auto* transform = scene_.getObject(objectId).getComponent<Transform>();
    if (!transform) return result;

    bool isLocal = rendererSettings_.isLocalTransformEnabled();
    glm::vec3 origin = transform->position;

    float dist = glm::length(cameraTransform.position - origin);
    float ringRadius = dist * 0.15f;
    float pickRadius = ringRadius * 0.18f;

    constexpr int segments = 32;

    const glm::vec3 colors[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const glm::vec3 perp1s[3] = {{0, 1, 0}, {1, 0, 0}, {1, 0, 0}};
    const glm::vec3 perp2s[3] = {{0, 0, 1}, {0, 0, 1}, {0, 1, 0}};

    result.pickingHandles.reserve(3 * segments);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 color = colors[i];
        glm::vec3 p1 = perp1s[i];
        glm::vec3 p2 = perp2s[i];
        if (isLocal) {
            p1 = rotateVector(p1, transform->rotation);
            p2 = rotateVector(p2, transform->rotation);
        }

        glm::vec3 prevPt{};
        for (int s = 0; s <= segments; ++s) {
            float angle = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
            glm::vec3 pt = origin + (p1 * glm::cos(angle) + p2 * glm::sin(angle)) * ringRadius;
            if (s > 0) {
                addGizmoLine(result.visualization, prevPt, pt, color);
                result.pickingHandles.emplace_back(
                        objectId, GizmoType::Rotation, static_cast<GizmoAxis>(i), prevPt, pt, pickRadius);
            }
            prevPt = pt;
        }
    }

    return result;
}

ObjectTransformHandle::GizmoTransformHandle
ObjectTransformHandle::buildScaleGizmo(EntityHandle objectId, const Camera& camera, const Transform& cameraTransform) {
    GizmoTransformHandle result;
    const auto* transform = scene_.getObject(objectId).getComponent<Transform>();
    if (!transform) return result;

    bool isLocal = rendererSettings_.isLocalTransformEnabled();
    glm::vec3 origin = transform->position;

    float dist = glm::length(cameraTransform.position - origin);
    float scale = dist * 0.15f;

    float shaftLength = scale;
    float cubeHalf = scale * 0.07f;
    float pickRadius = scale * 0.18f;
    float pickOffset = scale * 0.20f;

    const glm::vec3 axes[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const glm::vec3 colors[3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    const GizmoAxis gaxes[3] = {GizmoAxis::X, GizmoAxis::Y, GizmoAxis::Z};

    result.pickingHandles.reserve(4);

    for (int i = 0; i < 3; ++i) {
        glm::vec3 dir = axes[i];
        dir = rotateVector(dir, transform->rotation);
        glm::vec3 color = colors[i];
        glm::vec3 tip = origin + dir * shaftLength;

        addGizmoLine(result.visualization, origin, tip, color);
        auto cubeGizmo = buildGizmoCube(tip, cubeHalf, color);
        result.visualization.insert(result.visualization.end(), cubeGizmo.begin(), cubeGizmo.end());

        glm::vec3 capsuleBase = origin + dir * pickOffset;
        result.pickingHandles.emplace_back(
                objectId, GizmoType::Scale, gaxes[i], capsuleBase, tip + dir * cubeHalf, pickRadius);
    }

    float centerHalf = scale * 0.09f;
    auto centerCubeGizmo = buildGizmoCube(origin, centerHalf, {1, 1, 1});
    result.visualization.insert(result.visualization.end(), centerCubeGizmo.begin(), centerCubeGizmo.end());

    result.pickingHandles.emplace_back(objectId,
                                       GizmoType::Scale,
                                       GizmoAxis::All,
                                       origin - glm::vec3(0, centerHalf, 0),
                                       origin + glm::vec3(0, centerHalf, 0),
                                       centerHalf * 2.0f);

    return result;
}
