#pragma once
#include <functional>
#include <map>
#include <optional>
#include <string>
#include "../ClipboardService.hpp"
#include "../EditorSelection.hpp"
#include "../UndoHistory.hpp"
#include "modules/scene/EntityManager.hpp"

class SceneMutations {
public:
    SceneMutations(EntityManager& entityManager,
                   GameEngine& engine,
                   EditorSelection& editorSelection,
                   UndoHistory& undoHistory,
                   ClipboardService& clipboardService) :
        entityManager_(entityManager),
        engine_(engine),
        editorSelection_(editorSelection),
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
        editorSelection_.clearSelection();
        editorSelection_.addToSelection(e);
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

    // Selection-aware convenience methods for actions
    void copySelectedObject() {
        const auto& ids = editorSelection_.getSelectedEntityIds();
        if (ids.empty()) return;
        nlohmann::json arr = nlohmann::json::array();
        for (EntityHandle e: ids)
            arr.push_back(entityManager_.serializeToJson(e));
        clipboardService_.copy(arr, ClipboardPayloadType::Entity);
    }

    void cutSelectedObject() {
        copySelectedObject();
        deleteSelectedObjects();
    }

    void duplicateSelectedObjects() {
        const auto ids = editorSelection_.getSelectedEntityIds();
        if (ids.empty()) return;
        std::vector<nlohmann::json> snapshots;
        snapshots.reserve(ids.size());
        for (EntityHandle e: ids)
            snapshots.push_back(entityManager_.serializeToJson(e));
        std::vector<EntityHandle> created;
        created.reserve(ids.size());
        std::vector<nlohmann::json> createdSnapshots;
        createdSnapshots.reserve(ids.size());
        for (const auto& snapshot: snapshots) {
            EntityHandle e = entityManager_.upsertFromJson(snapshot);
            created.push_back(e);
            createdSnapshots.push_back(entityManager_.serializeToJson(e));
        }
        editorSelection_.clearSelection();
        for (EntityHandle e: created)
            editorSelection_.addToSelection(e);
        undoHistory_.push(
                [this, created] {
                    for (EntityHandle e: created)
                        entityManager_.destroyEntity(e);
                },
                [this, created, createdSnapshots] {
                    for (std::size_t i = 0; i < created.size(); ++i)
                        entityManager_.upsertFromJson(createdSnapshots[i], created[i]);
                });
    }

    void duplicateSelectedObject() { duplicateSelectedObjects(); }

    void pasteObject() {
        if (!clipboardService_.hasEntity()) return;
        const auto& clipboard = clipboardService_.get();
        if (!clipboard) return;
        if (clipboard->is_array()) {
            std::vector<EntityHandle> created;
            std::vector<nlohmann::json> snapshots;
            created.reserve(clipboard->size());
            snapshots.reserve(clipboard->size());
            for (const auto& item: *clipboard) {
                EntityHandle e = entityManager_.upsertFromJson(item);
                created.push_back(e);
                snapshots.push_back(entityManager_.serializeToJson(e));
            }
            editorSelection_.clearSelection();
            for (EntityHandle e: created)
                editorSelection_.addToSelection(e);
            undoHistory_.push(
                    [this, created] {
                        for (EntityHandle e: created)
                            entityManager_.destroyEntity(e);
                    },
                    [this, created, snapshots] {
                        for (std::size_t i = 0; i < created.size(); ++i)
                            entityManager_.upsertFromJson(snapshots[i], created[i]);
                    });
        } else if (clipboard->is_object() && !clipboard->empty()) {
            createObject(*clipboard);
        }
    }

    void deleteSelectedObjects() {
        const auto ids = editorSelection_.getSelectedEntityIds();
        if (ids.empty()) return;

        std::vector<nlohmann::json> snapshots;
        snapshots.reserve(ids.size());
        for (EntityHandle e: ids)
            snapshots.push_back(entityManager_.serializeToJson(e));
        for (EntityHandle e: ids)
            entityManager_.destroyEntity(e);
        editorSelection_.clearSelection();
        undoHistory_.push(
                [this, ids, snapshots] {
                    for (std::size_t i = 0; i < ids.size(); ++i)
                        entityManager_.upsertFromJson(snapshots[i], ids[i]);
                },
                [this, ids] {
                    for (EntityHandle e: ids)
                        entityManager_.destroyEntity(e);
                });
    }

    void deleteSelectedObject() { deleteSelectedObjects(); }

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
    EditorSelection& editorSelection_;
    UndoHistory& undoHistory_;
    ClipboardService& clipboardService_;

    std::optional<std::pair<EntityHandle, nlohmann::json>> pendingSnapshot_;

    void clearSelectionIfSelected(EntityHandle deletedId) {
        if (editorSelection_.isEntitySelected(deletedId)) {
            editorSelection_.clearSelection();
        }
    }
};
