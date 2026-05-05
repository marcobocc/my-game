#include "SceneMutations.hpp"
#include "ObjectSelection.hpp"
#include "data/components/BoxCollider.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Renderer.hpp"
#include "data/components/Transform.hpp"

std::string SceneMutations::createEmptyObjectAndSelect(const std::string& name) {
    auto id = createEmptyObject(name);
    if (objectSelection_) objectSelection_->selectObject(id);
    return id;
}

std::string SceneMutations::createModelAndSelect(const std::string& modelName,
                                                 const Scene::_createModel_Options& options) {
    auto id = createModel(modelName, options);
    if (objectSelection_) objectSelection_->selectObject(id);
    return id;
}

void SceneMutations::addComponentByType(const std::string& objectId, const std::string& componentType) {
    auto obj = scene_.getObject(objectId);
    if (componentType == "Transform" && !obj.has<Transform>()) {
        addComponent<Transform>(obj);
    } else if (componentType == "Camera" && !obj.has<Camera>()) {
        addComponent<Camera>(obj);
    } else if (componentType == "Renderer" && !obj.has<Renderer>()) {
        addComponent<Renderer>(obj);
    } else if (componentType == "BoxCollider" && !obj.has<BoxCollider>()) {
        addComponent<BoxCollider>(obj);
    }
}

void SceneMutations::removeComponentByType(const std::string& objectId, const std::string& componentType) {
    auto obj = scene_.getObject(objectId);
    if (componentType == "Transform" && obj.has<Transform>()) {
        removeComponent<Transform>(obj);
    } else if (componentType == "Camera" && obj.has<Camera>()) {
        removeComponent<Camera>(obj);
    } else if (componentType == "Renderer" && obj.has<Renderer>()) {
        removeComponent<Renderer>(obj);
    } else if (componentType == "BoxCollider" && obj.has<BoxCollider>()) {
        removeComponent<BoxCollider>(obj);
    }
}

void SceneMutations::beginEditByType(const std::string& objectId, const std::string& componentType) {
    auto obj = scene_.getObject(objectId);
    if (componentType == "Transform" && obj.has<Transform>()) {
        beginEdit(obj.get<Transform>());
    } else if (componentType == "Camera" && obj.has<Camera>()) {
        beginEdit(obj.get<Camera>());
    } else if (componentType == "BoxCollider" && obj.has<BoxCollider>()) {
        beginEdit(obj.get<BoxCollider>());
    } else if (componentType == "Renderer" && obj.has<Renderer>()) {
        beginEdit(obj.get<Renderer>());
    }
}

void SceneMutations::endEditByType(const std::string& objectId, const std::string& componentType) {
    auto obj = scene_.getObject(objectId);
    if (componentType == "Transform" && obj.has<Transform>()) {
        commitEdit(obj.get<Transform>());
    } else if (componentType == "Camera" && obj.has<Camera>()) {
        commitEdit(obj.get<Camera>());
    } else if (componentType == "BoxCollider" && obj.has<BoxCollider>()) {
        commitEdit(obj.get<BoxCollider>());
    } else if (componentType == "Renderer" && obj.has<Renderer>()) {
        commitEdit(obj.get<Renderer>());
    }
}

void SceneMutations::clearSelectionIfDeleted(const std::string& deletedId) {
    if (objectSelection_ && objectSelection_->getSelectedObjectId() == deletedId) {
        objectSelection_->clearSelection();
    }
}
