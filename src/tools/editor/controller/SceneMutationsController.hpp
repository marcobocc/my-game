#pragma once
#include <any>
#include <functional>
#include <string>
#include "GameEngine.hpp"
#include "UndoHistoryController.hpp"
#include "systems/scene/GameObject.hpp"

class SceneMutationsController {
public:
    explicit SceneMutationsController(GameEngine& engine) : engine_(engine) {}

    UndoHistoryController& undoHistory() { return undoHistory_; }

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
        undoHistory_.push([this, name] { engine_.getObject(name).remove<T>(); },
                          [this, name, val] {
                              auto& o = engine_.getObject(name);
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
                    auto& o = engine_.getObject(name);
                    o.add<T>();
                    o.get<T>() = val;
                },
                [this, name] { engine_.getObject(name).remove<T>(); });
    }

    std::string createCube(const Scene::_createMesh_Options& options) {
        std::string id = engine_.createCube(options);
        undoHistory_.push([this, id] { engine_.destroyObject(id); }, [this, options] { engine_.createCube(options); });
        return id;
    }

    std::string createRectangle2D(const Scene::_createMesh_Options& options) {
        std::string id = engine_.createRectangle2D(options);
        undoHistory_.push([this, id] { engine_.destroyObject(id); },
                          [this, options] { engine_.createRectangle2D(options); });
        return id;
    }

    std::string createModel(const std::string& modelName, const Scene::_createModel_Options& options) {
        std::string id = engine_.createModel(modelName, options);
        undoHistory_.push([this, id] { engine_.destroyObject(id); },
                          [this, modelName, options] { engine_.createModel(modelName, options); });
        return id;
    }

    std::string createEmptyObject(const std::string& name = "") {
        auto [id, success] = engine_.createEmptyObject(name);
        undoHistory_.push([this, id] { engine_.destroyObject(id); },
                          [this, name] { auto [newId, _] = engine_.createEmptyObject(name); });
        return id;
    }

private:
    GameEngine& engine_;
    UndoHistoryController undoHistory_;

    template<typename T>
    struct Snapshot {
        T* target;
        T oldValue;
    };
    std::any pendingOldValue_;
};
