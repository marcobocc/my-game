#include "SceneMutations.hpp"
#include "../ObjectSelection.hpp"

void SceneMutations::addComponentByType(EntityHandle entity, const std::string& componentType) {
    if (componentType == "Transform" && !entityManager_.hasComponent<Transform>(entity)) {
        entityManager_.upsertComponent<Transform>(entity);
    } else if (componentType == "Camera" && !entityManager_.hasComponent<Camera>(entity)) {
        entityManager_.upsertComponent<Camera>(entity);
    } else if (componentType == "Renderer" && !entityManager_.hasComponent<Renderer>(entity)) {
        entityManager_.upsertComponent<Renderer>(entity);
    } else if (componentType == "BoxCollider" && !entityManager_.hasComponent<BoxCollider>(entity)) {
        entityManager_.upsertComponent<BoxCollider>(entity);
    }
}

void SceneMutations::removeComponentByType(EntityHandle entity, const std::string& componentType) {
    if (componentType == "Transform" && entityManager_.hasComponent<Transform>(entity)) {
        entityManager_.removeComponent<Transform>(entity);
    } else if (componentType == "Camera" && entityManager_.hasComponent<Camera>(entity)) {
        entityManager_.removeComponent<Camera>(entity);
    } else if (componentType == "Renderer" && entityManager_.hasComponent<Renderer>(entity)) {
        entityManager_.removeComponent<Renderer>(entity);
    } else if (componentType == "BoxCollider" && entityManager_.hasComponent<BoxCollider>(entity)) {
        entityManager_.removeComponent<BoxCollider>(entity);
    }
}

void SceneMutations::clearSelectionIfSelected(EntityHandle deletedId) {
    auto selected = objectSelection_.getSelectedEntityId();
    if (selected && *selected == deletedId) {
        objectSelection_.clearSelection();
    }
}
