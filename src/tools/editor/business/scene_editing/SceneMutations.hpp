#pragma once
#include <optional>
#include "../../../../engine/GameEngine.hpp"
#include "UndoHistory.hpp"
#include "modules/scene/EntityManager.hpp"

class ObjectSelection;

class SceneMutations {
public:
    SceneMutations(EntityManager& entityManager, GameEngine& engine, ObjectSelection& objectSelection) :
        entityManager_(entityManager),
        engine_(engine),
        objectSelection_(objectSelection) {}

    void beginEdit(EntityHandle entity) {
        if (!pendingSnapshot_.has_value()) pendingSnapshot_.emplace(entity, entityManager_.serializeToJson(entity));
    }

    void commitEdit(EntityHandle entity) {
        if (!pendingSnapshot_.has_value()) return;
        if (pendingSnapshot_->first != entity) return;
        nlohmann::json before = pendingSnapshot_->second;
        nlohmann::json after = entityManager_.serializeToJson(entity);
        undoHistory_.push([this, entity, before] { entityManager_.replaceFromJson(entity, before); },
                          [this, entity, after] { entityManager_.replaceFromJson(entity, after); });
        pendingSnapshot_.reset();
    }

    EntityHandle createObject(const nlohmann::json& objectData) {
        EntityHandle e = entityManager_.createFromJson(objectData);
        nlohmann::json snapshot = entityManager_.serializeToJson(e);
        undoHistory_.push([this, e] { entityManager_.destroyEntity(e); },
                          [this, e, snapshot] { entityManager_.createFromJson(snapshot, e); });
        return e;
    }

    void destroyObject(EntityHandle entity) {
        nlohmann::json snapshot = entityManager_.serializeToJson(entity);
        entityManager_.destroyEntity(entity);
        undoHistory_.push([this, entity, snapshot] { entityManager_.createFromJson(snapshot, entity); },
                          [this, entity] { entityManager_.destroyEntity(entity); });
        clearSelectionIfSelected(entity);
    }

    void undo() { undoHistory_.undo(); }
    void redo() { undoHistory_.redo(); }

    void addComponentByType(EntityHandle entity, const std::string& componentType);
    void removeComponentByType(EntityHandle entity, const std::string& componentType);

private:
    EntityManager& entityManager_;
    GameEngine& engine_;
    ObjectSelection& objectSelection_;

    UndoHistory undoHistory_;
    std::optional<std::pair<EntityHandle, nlohmann::json>> pendingSnapshot_;

    void clearSelectionIfSelected(EntityHandle deletedId);
};
