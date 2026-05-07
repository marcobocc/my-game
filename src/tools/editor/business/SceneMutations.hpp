#pragma once
#include <any>
#include <functional>
#include <string>
#include "../../../engine/GameEngine.hpp"
#include "UndoHistory.hpp"
#include "modules/scene/GameObject.hpp"
#include "modules/scene/Scene.hpp"

class ObjectSelection;

class SceneMutations {
public:
    SceneMutations(Scene& scene, GameEngine& engine, ObjectSelection* objectSelection = nullptr) :
        scene_(scene),
        engine_(engine),
        objectSelection_(objectSelection) {}

    UndoHistory& undoHistory() { return undoHistory_; }

    template<typename T>
    void beginEdit(T& target) {
        pendingOldValue_ = Snapshot<T>{&target, target};
    }

    template<typename T>
    void commitEdit(T& target) {
        if (!pendingOldValue_.has_value()) return;
        auto* snap = std::any_cast<Snapshot<T>>(&pendingOldValue_);
        if (!snap || snap->target != &target) return;
        T oldVal = snap->oldValue;
        T newVal = target;
        T* ptr = &target;
        undoHistory_.push([ptr, oldVal] { *ptr = oldVal; }, [ptr, newVal] { *ptr = newVal; });
        pendingOldValue_.reset();
    }

    template<typename T>
    void addComponent(GameObject& obj) {
        std::string name = obj.getName();
        obj.add<T>();
        T val = obj.get<T>();
        undoHistory_.push([this, name] { scene_.getObject(name).remove<T>(); },
                          [this, name, val] {
                              auto& o = scene_.getObject(name);
                              o.add<T>();
                              o.get<T>() = val;
                          });
    }

    template<typename T>
    void removeComponent(GameObject& obj) {
        std::string name = obj.getName();
        T val = obj.get<T>();
        obj.remove<T>();
        undoHistory_.push(
                [this, name, val] {
                    auto& o = scene_.getObject(name);
                    o.add<T>();
                    o.get<T>() = val;
                },
                [this, name] { scene_.getObject(name).remove<T>(); });
    }

    std::string createCube(const Scene::_createMesh_Options& options) {
        std::string id = scene_.createCube(options);
        undoHistory_.push([this, id] { scene_.destroyObject(id); }, [this, options] { scene_.createCube(options); });
        return id;
    }

    std::string createPlane(const Scene::_createMesh_Options& options) {
        std::string id = scene_.createPlane(options);
        undoHistory_.push([this, id] { scene_.destroyObject(id); }, [this, options] { scene_.createPlane(options); });
        return id;
    }

    std::string createSphere(const Scene::_createMesh_Options& options) {
        std::string id = scene_.createMesh("_PRIMITIVE_SPHERE_6", options);
        undoHistory_.push([this, id] { scene_.destroyObject(id); }, [this, options] { scene_.createPlane(options); });
        return id;
    }

    std::string createModel(const std::string& modelName, const Scene::_createModel_Options& options) {
        std::string id = scene_.createModel(modelName, options);
        undoHistory_.push([this, id] { scene_.destroyObject(id); },
                          [this, modelName, options] { scene_.createModel(modelName, options); });
        return id;
    }

    std::string createEmptyObject(const std::string& name = "") {
        auto [id, success] = scene_.createEmptyObject(name);
        undoHistory_.push([this, id] { scene_.destroyObject(id); },
                          [this, name] { auto [newId, _] = scene_.createEmptyObject(name); });
        return id;
    }

    std::string deleteObject(const std::string& name) {
        auto obj = scene_.getObject(name);
        auto snapshot = scene_.serializeGameObject(obj);
        scene_.destroyObject(name);
        undoHistory_.push([this, snapshot, name] { scene_.createGameObjectFromJson(snapshot, name); },
                          [this, name] { scene_.destroyObject(name); });
        if (objectSelection_) clearSelectionIfDeleted(name);
        return name;
    }

    std::string createEmptyObjectAndSelect(const std::string& name = "");
    std::string createModelAndSelect(const std::string& modelName, const Scene::_createModel_Options& options);

    void addComponentByType(const std::string& objectId, const std::string& componentType);
    void removeComponentByType(const std::string& objectId, const std::string& componentType);
    void beginEditByType(const std::string& objectId, const std::string& componentType);
    void endEditByType(const std::string& objectId, const std::string& componentType);


private:
    Scene& scene_;
    GameEngine& engine_;
    ObjectSelection* objectSelection_;
    UndoHistory undoHistory_;

    void clearSelectionIfDeleted(const std::string& deletedId);

    template<typename T>
    struct Snapshot {
        T* target;
        T oldValue;
    };
    std::any pendingOldValue_;
};
