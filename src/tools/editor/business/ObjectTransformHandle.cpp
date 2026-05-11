#include "ObjectTransformHandle.hpp"
#include <string>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/modules/input/RaycastPickingSystem.hpp"
#include "../../../engine/modules/scene/components/Transform.hpp"
#include "EditorCamera.hpp"
#include "EditorSettings.hpp"
#include "ObjectSelection.hpp"
#include "scene_editing/SceneMutations.hpp"

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
    auto selectedId = objectSelection_.getSelectedEntityId();
    if (!selectedId) return;

    auto* transformPtr = entityManager_.getComponent<Transform>(*selectedId);
    if (!transformPtr) return;

    glm::vec3 axisDir = transformAxisToLocalSpace(axis, *transformPtr);
    glm::vec3 origin = transformPtr->position;

    glm::vec3 camDir = glm::normalize(editorOrbitCamera_.getCameraTransform().position - origin);
    glm::vec3 planeNormal = glm::normalize(camDir - glm::dot(camDir, axisDir) * axisDir);
    if (glm::length(planeNormal) < 1e-5f) planeNormal = editorOrbitCamera_.getCameraTransform().getUp();

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, origin, planeNormal);
    if (!hitPoint) return;

    sceneMutations_.beginEdit(*selectedId);

    dragState_.activeDrag.emplace(*selectedId, axis, axisDir, planeNormal, origin, *hitPoint);
}

void ObjectTransformHandle::updateTranslationDrag(double mouseX, double mouseY) {
    if (!dragState_.activeDrag) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(dragState_.activeDrag->objectId);
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

    transformPtr->position = pos;
}

void ObjectTransformHandle::commitTranslationDrag() {
    if (!dragState_.activeDrag) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(dragState_.activeDrag->objectId);
    if (transformPtr) sceneMutations_.commitEdit(dragState_.activeDrag->objectId);
    dragState_.activeDrag.reset();
}

void ObjectTransformHandle::beginRotationDrag(GizmoAxis axis, double mouseX, double mouseY) {
    auto selectedId = objectSelection_.getSelectedEntityId();
    if (!selectedId) return;

    auto* transformPtr = entityManager_.getComponent<Transform>(*selectedId);
    if (!transformPtr) return;

    glm::vec3 axisDir = transformAxisToLocalSpace(axis, *transformPtr);
    glm::vec3 origin = transformPtr->position;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint = rayPlaneIntersect(ray, origin, axisDir);
    if (!hitPoint) return;

    glm::vec3 hitDir = *hitPoint - origin;
    if (glm::length(hitDir) < 1e-5f) return;

    hitDir = glm::normalize(hitDir);

    sceneMutations_.beginEdit(*selectedId);
    dragState_.activeRotationDrag.emplace(*selectedId, axis, axisDir, origin, transformPtr->rotation, hitDir);
}

void ObjectTransformHandle::updateRotationDrag(double mouseX, double mouseY) {
    if (!dragState_.activeRotationDrag) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(dragState_.activeRotationDrag->objectId);
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

    transformPtr->rotation = glm::angleAxis(angle, dragState_.activeRotationDrag->axisDir) *
                             dragState_.activeRotationDrag->initialRotation;
}

void ObjectTransformHandle::commitRotationDrag() {
    if (!dragState_.activeRotationDrag) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(dragState_.activeRotationDrag->objectId);
    if (transformPtr) sceneMutations_.commitEdit(dragState_.activeRotationDrag->objectId);
    dragState_.activeRotationDrag.reset();
}

void ObjectTransformHandle::beginScaleDrag(GizmoAxis axis, double mouseX, double mouseY) {
    auto selectedId = objectSelection_.getSelectedEntityId();
    if (!selectedId) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(*selectedId);
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

    sceneMutations_.beginEdit(*selectedId);
    dragState_.activeScaleDrag.emplace(*selectedId, axis, axisDir, origin, transformPtr->scale, planeNormal, initialT);
}

void ObjectTransformHandle::updateScaleDrag(double mouseX, double mouseY) {
    if (!dragState_.activeScaleDrag) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(dragState_.activeScaleDrag->objectId);
    if (!transformPtr) return;

    Ray ray = buildMouseRay(mouseX, mouseY);
    auto hitPoint =
            rayPlaneIntersect(ray, dragState_.activeScaleDrag->origin, dragState_.activeScaleDrag->dragPlaneNormal);
    if (!hitPoint) return;

    float currentT = glm::dot(*hitPoint - dragState_.activeScaleDrag->origin, dragState_.activeScaleDrag->axisDir);
    float factor = glm::max(currentT / dragState_.activeScaleDrag->initialT, 0.01f);

    if (dragState_.activeScaleDrag->axis == GizmoAxis::All) {
        transformPtr->scale = dragState_.activeScaleDrag->initialScale * factor;
    } else {
        glm::vec3 newScale = dragState_.activeScaleDrag->initialScale;
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
        transformPtr->scale = newScale;
    }
}

void ObjectTransformHandle::commitScaleDrag() {
    if (!dragState_.activeScaleDrag) return;
    auto* transformPtr = entityManager_.getComponent<Transform>(dragState_.activeScaleDrag->objectId);
    if (transformPtr) sceneMutations_.commitEdit(dragState_.activeScaleDrag->objectId);
    dragState_.activeScaleDrag.reset();
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
