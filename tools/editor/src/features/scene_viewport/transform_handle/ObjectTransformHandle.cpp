#include "ObjectTransformHandle.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/EditorSettings.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../editor_camera/EditorCamera.hpp"
#include "modules/input/RaycastPickingSystem.hpp"
#include "modules/scene/TransformUtils.hpp"
#include "modules/scene/components/Transform.hpp"

// Walks the parent chain and returns the world matrix of t's parent (identity if no parent).
static glm::mat4 parentWorldMatrix(const Transform& t, const RuntimeScene& scene) {
    if (t.parent == INVALID_ENTITY_HANDLE) return glm::mat4(1.0f);
    const auto* parentTransform = scene.getObject(t.parent).getComponent<Transform>();
    if (!parentTransform) return glm::mat4(1.0f);
    glm::mat4 result = glm::mat4(1.0f);
    const Transform* cur = parentTransform;
    std::vector<const Transform*> chain;
    for (int i = 0; i < 64 && cur; ++i) {
        chain.push_back(cur);
        if (cur->parent == INVALID_ENTITY_HANDLE) break;
        cur = scene.getObject(cur->parent).getComponent<Transform>();
    }
    for (auto it = chain.rbegin(); it != chain.rend(); ++it)
        result = result * (*it)->getModelMatrix();
    return result;
}

// Returns the world-space rotation of t (combines parent chain rotations with local rotation).
static glm::quat worldRotation(const Transform& t, const RuntimeScene& scene) {
    glm::mat4 parentMat = parentWorldMatrix(t, scene);
    glm::mat3 rotPart = glm::mat3(parentMat);
    // Strip scale from each column
    glm::vec3 col0 = glm::normalize(rotPart[0]);
    glm::vec3 col1 = glm::normalize(rotPart[1]);
    glm::vec3 col2 = glm::normalize(rotPart[2]);
    glm::quat parentRot = glm::quat_cast(glm::mat3(col0, col1, col2));
    return parentRot * t.rotation;
}

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
    glm::quat rot = worldRotation(transform, scene_);
    return glm::normalize(rot * axisToDir(axis));
}

glm::vec3 ObjectTransformHandle::computeGroupPivot(const std::vector<EntityHandle>& ids) const {
    glm::vec3 sum{0};
    int count = 0;
    for (const auto& id: ids) {
        const auto* t = scene_.getObject(id).getComponent<Transform>();
        if (t) {
            glm::mat4 parentMat = parentWorldMatrix(*t, scene_);
            sum += glm::vec3(parentMat * glm::vec4(t->position, 1.0f));
            ++count;
        }
    }
    return count > 0 ? sum / static_cast<float>(count) : glm::vec3{0};
}

void ObjectTransformHandle::beginTranslationDrag(GizmoAxis axis, double mouseX, double mouseY) {
    const auto& ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;

    glm::vec3 pivot = computeGroupPivot(ids);

    glm::vec3 axisDir = axisToDir(axis);
    if (ids.size() == 1) {
        const auto* t = scene_.getObject(ids[0]).getComponent<Transform>();
        if (t) axisDir = transformAxisToLocalSpace(axis, *t);
    }

    glm::vec3 camDir = glm::normalize(editorOrbitCamera_.getCameraTransform().position - pivot);
    glm::vec3 planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
    if (glm::length(planeNormal) < 1e-5f) planeNormal = editorOrbitCamera_.getCameraTransform().getUp();

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, pivot, planeNormal);
    if (!hitPoint) return;

    dragGroupId_ = UndoHistory::randomGroupId("Translate");
    prevTranslationDelta_ = glm::vec3{0.0f};

    ActiveDrag drag;
    drag.type = GizmoType::Translation;
    drag.axis = axis;
    drag.pivot = pivot;
    drag.axisDir = axisDir;
    drag.dragPlaneNormal = planeNormal;
    drag.initialHitPoint = *hitPoint;
    dragState_.activeDrag = drag;
}

void ObjectTransformHandle::updateTranslationDrag(double mouseX, double mouseY) {
    if (!dragState_.activeDrag) return;
    const auto& drag = *dragState_.activeDrag;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, drag.initialHitPoint, drag.dragPlaneNormal);
    if (!hitPoint) return;

    float startT = projectOnAxis(drag.initialHitPoint, drag.pivot, drag.axisDir);
    float currentT = projectOnAxis(*hitPoint, drag.pivot, drag.axisDir);
    glm::vec3 currentDelta = drag.axisDir * (currentT - startT);

    if (rendererSettings_.isGridSnappingEnabled()) {
        float scale = rendererSettings_.getGridScale();
        currentDelta = glm::round(currentDelta / scale) * scale;
    }

    glm::vec3 frameDelta = currentDelta - prevTranslationDelta_;
    prevTranslationDelta_ = currentDelta;

    for (const auto& id: editorSelection_.getSelectedEntityIds()) {
        const auto* t = scene_.getObject(id).getComponent<Transform>();
        if (!t) continue;
        glm::mat4 parentMat = parentWorldMatrix(*t, scene_);
        glm::vec3 localDelta = glm::vec3(glm::inverse(parentMat) * glm::vec4(frameDelta, 0.0f));
        glm::vec3 newPos = t->position + localDelta;
        scene_.getObject(id).mutateComponent<Transform>([newPos](Transform& t) { t.position = newPos; }, dragGroupId_);
    }
}

void ObjectTransformHandle::commitTranslationDrag() {
    dragState_.activeDrag.reset();
    dragGroupId_.clear();
}

void ObjectTransformHandle::beginRotationDrag(GizmoAxis axis, double mouseX, double mouseY) {
    const auto& ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;

    glm::vec3 pivot = computeGroupPivot(ids);

    glm::vec3 axisDir = axisToDir(axis);
    if (ids.size() == 1) {
        const auto* t = scene_.getObject(ids[0]).getComponent<Transform>();
        if (t) axisDir = transformAxisToLocalSpace(axis, *t);
    }

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, pivot, axisDir);
    if (!hitPoint) return;

    glm::vec3 hitDir = *hitPoint - pivot;
    if (glm::length(hitDir) < 1e-5f) return;
    hitDir = glm::normalize(hitDir);

    dragGroupId_ = UndoHistory::randomGroupId("Rotate");
    prevRotationAngle_ = 0.0f;

    ActiveDrag drag;
    drag.type = GizmoType::Rotation;
    drag.axis = axis;
    drag.pivot = pivot;
    drag.axisDir = axisDir;
    drag.initialHitDir = hitDir;
    dragState_.activeDrag = drag;
}

void ObjectTransformHandle::updateRotationDrag(double mouseX, double mouseY) {
    if (!dragState_.activeDrag) return;
    const auto& drag = *dragState_.activeDrag;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, drag.pivot, drag.axisDir);
    if (!hitPoint) return;

    glm::vec3 hitDir = *hitPoint - drag.pivot;
    if (glm::length(hitDir) < 1e-5f) return;
    hitDir = glm::normalize(hitDir);

    float cosA = glm::clamp(glm::dot(drag.initialHitDir, hitDir), -1.0f, 1.0f);
    float currentAngle = glm::acos(cosA);
    if (glm::dot(glm::cross(drag.initialHitDir, hitDir), drag.axisDir) < 0.0f) currentAngle = -currentAngle;

    float frameDeltaAngle = currentAngle - prevRotationAngle_;
    prevRotationAngle_ = currentAngle;

    glm::quat deltaRot = glm::angleAxis(frameDeltaAngle, drag.axisDir);

    for (const auto& id: editorSelection_.getSelectedEntityIds()) {
        const auto* t = scene_.getObject(id).getComponent<Transform>();
        if (!t) continue;
        glm::mat4 parentMat = parentWorldMatrix(*t, scene_);
        glm::mat4 invParent = glm::inverse(parentMat);
        // Compute world-space position after rotation, then bring back to local space.
        glm::vec3 worldPos = glm::vec3(parentMat * glm::vec4(t->position, 1.0f));
        glm::vec3 newWorldPos = drag.pivot + deltaRot * (worldPos - drag.pivot);
        glm::vec3 newPos = glm::vec3(invParent * glm::vec4(newWorldPos, 1.0f));
        // Rotation: remove parent's rotation, apply delta, re-apply parent's rotation inverse.
        glm::quat parentRot = glm::quat_cast(parentMat);
        glm::quat newRot = glm::inverse(parentRot) * deltaRot * parentRot * t->rotation;
        scene_.getObject(id).mutateComponent<Transform>(
                [newPos, newRot](Transform& t) {
                    t.position = newPos;
                    t.rotation = newRot;
                },
                dragGroupId_);
    }
}

void ObjectTransformHandle::commitRotationDrag() {
    dragState_.activeDrag.reset();
    dragGroupId_.clear();
}

void ObjectTransformHandle::beginScaleDrag(GizmoAxis axis, double mouseX, double mouseY) {
    const auto& ids = editorSelection_.getSelectedEntityIds();
    if (ids.empty()) return;

    glm::vec3 pivot = computeGroupPivot(ids);

    glm::vec3 axisDir;
    glm::vec3 planeNormal;
    if (axis == GizmoAxis::All) {
        axisDir = glm::normalize(editorOrbitCamera_.getCameraTransform().getRight());
        planeNormal = glm::normalize(editorOrbitCamera_.getCameraTransform().position - pivot);
    } else {
        glm::vec3 worldAxis = axisToDir(axis);
        if (ids.size() == 1) {
            const auto* t = scene_.getObject(ids[0]).getComponent<Transform>();
            if (t) {
                glm::mat3 rotationMatrix = glm::mat3_cast(t->rotation);
                worldAxis = glm::normalize(rotationMatrix * worldAxis);
            }
        }
        axisDir = worldAxis;
        glm::vec3 camDir = glm::normalize(editorOrbitCamera_.getCameraTransform().position - pivot);
        planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
        if (glm::length(planeNormal) < 1e-5f) planeNormal = editorOrbitCamera_.getCameraTransform().getUp();
    }

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, pivot, planeNormal);
    if (!hitPoint) return;

    float initialT = glm::dot(*hitPoint - pivot, axisDir);
    if (std::abs(initialT) < 1e-5f) return;

    dragGroupId_ = UndoHistory::randomGroupId("Scale");
    prevScaleFactor_ = 1.0f;

    ActiveDrag drag;
    drag.type = GizmoType::Scale;
    drag.axis = axis;
    drag.pivot = pivot;
    drag.axisDir = axisDir;
    drag.dragPlaneNormal = planeNormal;
    drag.initialT = initialT;
    dragState_.activeDrag = drag;
}

void ObjectTransformHandle::updateScaleDrag(double mouseX, double mouseY) {
    if (!dragState_.activeDrag) return;
    const auto& drag = *dragState_.activeDrag;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, drag.pivot, drag.dragPlaneNormal);
    if (!hitPoint) return;

    float currentT = glm::dot(*hitPoint - drag.pivot, drag.axisDir);
    float currentFactor = glm::max(currentT / drag.initialT, 0.01f);
    float frameFactor = currentFactor / prevScaleFactor_;
    prevScaleFactor_ = currentFactor;

    for (const auto& id: editorSelection_.getSelectedEntityIds()) {
        const auto* t = scene_.getObject(id).getComponent<Transform>();
        if (!t) continue;

        glm::mat4 parentMat = parentWorldMatrix(*t, scene_);
        glm::mat4 invParent = glm::inverse(parentMat);
        glm::vec3 worldPos = glm::vec3(parentMat * glm::vec4(t->position, 1.0f));
        glm::vec3 offset = worldPos - drag.pivot;
        glm::vec3 newWorldPos;
        glm::vec3 newScale = t->scale;

        if (drag.axis == GizmoAxis::All) {
            newWorldPos = drag.pivot + offset * frameFactor;
            newScale = t->scale * frameFactor;
        } else {
            glm::vec3 axisUnit = drag.axisDir;
            float along = glm::dot(offset, axisUnit);
            newWorldPos = worldPos + axisUnit * (along * (frameFactor - 1.0f));
            switch (drag.axis) {
                case GizmoAxis::X:
                    newScale.x = t->scale.x * frameFactor;
                    break;
                case GizmoAxis::Y:
                    newScale.y = t->scale.y * frameFactor;
                    break;
                case GizmoAxis::Z:
                    newScale.z = t->scale.z * frameFactor;
                    break;
                default:
                    break;
            }
        }

        glm::vec3 newPos = glm::vec3(invParent * glm::vec4(newWorldPos, 1.0f));
        scene_.getObject(id).mutateComponent<Transform>(
                [newPos, newScale](Transform& t) {
                    t.position = newPos;
                    t.scale = newScale;
                },
                dragGroupId_);
    }
}

void ObjectTransformHandle::commitScaleDrag() {
    dragState_.activeDrag.reset();
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
    if (!dragState_.activeDrag) return;
    switch (dragState_.activeDrag->type) {
        case GizmoType::Translation:
            updateTranslationDrag(mouseX, mouseY);
            break;
        case GizmoType::Rotation:
            updateRotationDrag(mouseX, mouseY);
            break;
        case GizmoType::Scale:
            updateScaleDrag(mouseX, mouseY);
            break;
    }
}

void ObjectTransformHandle::endDrag() {
    if (!dragState_.activeDrag) return;
    switch (dragState_.activeDrag->type) {
        case GizmoType::Translation:
            commitTranslationDrag();
            break;
        case GizmoType::Rotation:
            commitRotationDrag();
            break;
        case GizmoType::Scale:
            commitScaleDrag();
            break;
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

ObjectTransformHandle::GizmoTransformHandle
ObjectTransformHandle::buildTranslationGizmo(glm::vec3 pivot, const Camera& camera, const Transform& cameraTransform) {
    GizmoTransformHandle result;

    const auto& ids = editorSelection_.getSelectedEntityIds();

    bool isLocal = rendererSettings_.isLocalTransformEnabled() && ids.size() == 1;
    glm::quat localRot = glm::quat{1, 0, 0, 0};
    if (isLocal) {
        const auto* t = scene_.getObject(ids[0]).getComponent<Transform>();
        if (t) localRot = worldRotation(*t, scene_);
    }

    float dist = glm::length(cameraTransform.position - pivot);
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
        if (isLocal) dir = rotateVector(dir, localRot);
        glm::vec3 color = colors[i];
        glm::vec3 tip = pivot + dir * shaftLength;

        addGizmoLine(result.visualization, pivot, tip, color);

        glm::vec3 perp1 =
                glm::normalize(glm::cross(dir, glm::abs(dir.x) < 0.9f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0)));
        glm::vec3 perp2 = glm::cross(dir, perp1);
        glm::vec3 headBase = tip - dir * headLength;
        addGizmoLine(result.visualization, headBase + perp1 * headSpread, tip, color);
        addGizmoLine(result.visualization, headBase - perp1 * headSpread, tip, color);
        addGizmoLine(result.visualization, headBase + perp2 * headSpread, tip, color);
        addGizmoLine(result.visualization, headBase - perp2 * headSpread, tip, color);

        glm::vec3 capsuleBase = pivot + dir * pickOffset;
        result.pickingHandles.push_back({ids, GizmoType::Translation, gaxes[i], capsuleBase, tip, pickRadius});
    }

    return result;
}

ObjectTransformHandle::GizmoTransformHandle
ObjectTransformHandle::buildRotationGizmo(glm::vec3 pivot, const Camera& camera, const Transform& cameraTransform) {
    GizmoTransformHandle result;

    const auto& ids = editorSelection_.getSelectedEntityIds();

    bool isLocal = rendererSettings_.isLocalTransformEnabled() && ids.size() == 1;
    glm::quat localRot = glm::quat{1, 0, 0, 0};
    if (isLocal) {
        const auto* t = scene_.getObject(ids[0]).getComponent<Transform>();
        if (t) localRot = worldRotation(*t, scene_);
    }

    float dist = glm::length(cameraTransform.position - pivot);
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
            p1 = rotateVector(p1, localRot);
            p2 = rotateVector(p2, localRot);
        }

        glm::vec3 prevPt{};
        for (int s = 0; s <= segments; ++s) {
            float angle = glm::two_pi<float>() * static_cast<float>(s) / static_cast<float>(segments);
            glm::vec3 pt = pivot + (p1 * glm::cos(angle) + p2 * glm::sin(angle)) * ringRadius;
            if (s > 0) {
                addGizmoLine(result.visualization, prevPt, pt, color);
                result.pickingHandles.push_back(
                        {ids, GizmoType::Rotation, static_cast<GizmoAxis>(i), prevPt, pt, pickRadius});
            }
            prevPt = pt;
        }
    }

    return result;
}

ObjectTransformHandle::GizmoTransformHandle
ObjectTransformHandle::buildScaleGizmo(glm::vec3 pivot, const Camera& camera, const Transform& cameraTransform) {
    GizmoTransformHandle result;

    const auto& ids = editorSelection_.getSelectedEntityIds();

    float dist = glm::length(cameraTransform.position - pivot);
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
        if (ids.size() == 1) {
            const auto* t = scene_.getObject(ids[0]).getComponent<Transform>();
            if (t) dir = rotateVector(dir, worldRotation(*t, scene_));
        }
        glm::vec3 color = colors[i];
        glm::vec3 tip = pivot + dir * shaftLength;

        addGizmoLine(result.visualization, pivot, tip, color);
        auto cubeGizmo = buildGizmoCube(tip, cubeHalf, color);
        result.visualization.insert(result.visualization.end(), cubeGizmo.begin(), cubeGizmo.end());

        glm::vec3 capsuleBase = pivot + dir * pickOffset;
        result.pickingHandles.push_back(
                {ids, GizmoType::Scale, gaxes[i], capsuleBase, tip + dir * cubeHalf, pickRadius});
    }

    float centerHalf = scale * 0.09f;
    auto centerCubeGizmo = buildGizmoCube(pivot, centerHalf, {1, 1, 1});
    result.visualization.insert(result.visualization.end(), centerCubeGizmo.begin(), centerCubeGizmo.end());

    result.pickingHandles.push_back({ids,
                                     GizmoType::Scale,
                                     GizmoAxis::All,
                                     pivot - glm::vec3(0, centerHalf, 0),
                                     pivot + glm::vec3(0, centerHalf, 0),
                                     centerHalf * 2.0f});

    return result;
}
