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
        undoHistory_.push([this, entity, before] { entityManager_.upsertFromJson(before, entity); },
                          [this, entity, after] { entityManager_.upsertFromJson(after, entity); });
        pendingSnapshot_.reset();
    }

    EntityHandle createObject(const nlohmann::json& objectData) {
        EntityHandle e = entityManager_.upsertFromJson(objectData);
        nlohmann::json snapshot = entityManager_.serializeToJson(e);
        undoHistory_.push([this, e] { entityManager_.destroyEntity(e); },
                          [this, e, snapshot] { entityManager_.upsertFromJson(snapshot, e); });
        return e;
    }

    void destroyObject(EntityHandle entity) {
        nlohmann::json snapshot = entityManager_.serializeToJson(entity);
        entityManager_.destroyEntity(entity);
        undoHistory_.push([this, entity, snapshot] { entityManager_.upsertFromJson(snapshot, entity); },
                          [this, entity] { entityManager_.destroyEntity(entity); });
        clearSelectionIfSelected(entity);
    }

    void undo() { undoHistory_.undo(); }
    void redo() { undoHistory_.redo(); }

    template<typename Component>
    void addComponent(EntityHandle entity) {
        entityManager_.upsertComponent<Component>(entity);
    }

    template<typename Component>
    void removeComponent(EntityHandle entity) {
        entityManager_.removeComponent<Component>(entity);
    }

private:
    EntityManager& entityManager_;
    GameEngine& engine_;
    ObjectSelection& objectSelection_;

    UndoHistory undoHistory_;
    std::optional<std::pair<EntityHandle, nlohmann::json>> pendingSnapshot_;

    void clearSelectionIfSelected(EntityHandle deletedId);
};
