#pragma once
#include <functional>
#include <map>
#include <optional>
#include <string>
#include "../ClipboardService.hpp"
#include "../ObjectSelection.hpp"
#include "../UndoHistory.hpp"
#include "modules/scene/EntityManager.hpp"

class SceneMutations {
public:
    SceneMutations(EntityManager& entityManager,
                   GameEngine& engine,
                   ObjectSelection& objectSelection,
                   UndoHistory& undoHistory,
                   ClipboardService& clipboardService) :
        entityManager_(entityManager),
        engine_(engine),
        objectSelection_(objectSelection),
        undoHistory_(undoHistory),
        clipboardService_(clipboardService) {}

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
        objectSelection_.selectObject(e);
        return e;
    }

    void destroyObject(EntityHandle entity) {
        nlohmann::json snapshot = entityManager_.serializeToJson(entity);
        entityManager_.destroyEntity(entity);
        undoHistory_.push([this, entity, snapshot] { entityManager_.upsertFromJson(snapshot, entity); },
                          [this, entity] { entityManager_.destroyEntity(entity); });
        clearSelectionIfSelected(entity);
    }

    template<typename Component>
    void addComponent(EntityHandle entity) {
        entityManager_.upsertComponent<Component>(entity);
    }

    template<typename Component>
    void removeComponent(EntityHandle entity) {
        entityManager_.removeComponent<Component>(entity);
    }

    void duplicateObject(EntityHandle entity) {
        nlohmann::json serialized = entityManager_.serializeToJson(entity);
        createObject(serialized);
    }

    void copyObject(EntityHandle entity) {
        clipboardService_.copy(entityManager_.serializeToJson(entity), ClipboardPayloadType::Entity);
    }

    void cutObject(EntityHandle entity) {
        copyObject(entity);
        destroyObject(entity);
    }

    void pasteObject() {
        if (!clipboardService_.hasEntity()) return;
        auto clipboard = clipboardService_.get();
        if (!clipboard || !clipboard->is_object() || clipboard->empty()) return;
        createObject(*clipboard);
    }

    // Selection-aware convenience methods for actions
    void copySelectedObject() {
        auto selectedId = objectSelection_.getSelectedEntityId();
        if (selectedId.has_value()) copyObject(*selectedId);
    }

    void cutSelectedObject() {
        auto selectedId = objectSelection_.getSelectedEntityId();
        if (selectedId.has_value()) cutObject(*selectedId);
    }

    void duplicateSelectedObject() {
        auto selectedId = objectSelection_.getSelectedEntityId();
        if (selectedId.has_value()) duplicateObject(*selectedId);
    }

    void deleteSelectedObject() {
        auto selectedId = objectSelection_.getSelectedEntityId();
        if (selectedId.has_value()) destroyObject(*selectedId);
    }

    std::map<std::string, std::function<void()>> getObjectEditActions(EntityHandle entity) {
        return {{"Copy", [this, entity] { copyObject(entity); }},
                {"Cut", [this, entity] { cutObject(entity); }},
                {"Duplicate", [this, entity] { duplicateObject(entity); }},
                {"Paste", [this] { pasteObject(); }},
                {"Delete", [this, entity] { destroyObject(entity); }}};
    }

private:
    EntityManager& entityManager_;
    GameEngine& engine_;
    ObjectSelection& objectSelection_;
    UndoHistory& undoHistory_;
    ClipboardService& clipboardService_;

    std::optional<std::pair<EntityHandle, nlohmann::json>> pendingSnapshot_;

    void clearSelectionIfSelected(EntityHandle deletedId) {
        auto selected = objectSelection_.getSelectedEntityId();
        if (selected && *selected == deletedId) {
            objectSelection_.clearSelection();
        }
    }
};
