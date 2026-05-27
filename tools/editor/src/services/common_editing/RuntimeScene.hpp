#pragma once
#include "../UndoHistory.hpp"
#include "RuntimeGameObject.hpp"
#include "modules/scene/EntityStore.hpp"
#include "transport/GameObjectDTO.hpp"
#include "transport/SceneDTO.hpp"

/*
RuntimeScene

A live facade over the ECS EntityStore, responsible for entity lifecycle
management, serialization, and undo/redo integration.

It does not own data. Instead, it directly reflects and manipulates the
current state of the underlying EntityStore in real time.

Any operation performed through this class immediately affects the live ECS.

------------------------------------------------------------
Live Behavior
------------------------------------------------------------

Because this is a live facade:

- There is no staging or buffering of changes
- Entity creation/destruction is immediate
- Component modifications affect the active scene instantly
- Snapshots reflect the current ECS state at the moment of call

The facade always mirrors the EntityStore one-to-one.

------------------------------------------------------------
Undo/Redo Integration
------------------------------------------------------------

All mutations performed through this facade are recorded in UndoHistory.

Each operation stores:
- undo action (revert ECS state)
- redo action (reapply ECS state)
- optional group identifier for batching

Undo operations directly mutate the live EntityStore.

------------------------------------------------------------
Snapshot System
------------------------------------------------------------

The scene can be serialized into a SceneDTO representing the full
live ECS state at the time of capture.

Snapshots include:
- all active entities
- all attached components per entity
- full component state
*/
class RuntimeScene {
public:
    RuntimeScene(EntityManager& store, UndoHistory& undo) : store_(store), undo_(undo) {}

    RuntimeGameObject getObject(EntityHandle e) const;

    // --------------------------------------------------------------------------------
    // Snapshotting Operations
    // --------------------------------------------------------------------------------
    SceneDTO snapshotScene() const;
    GameObjectDTO snapshotObject(EntityHandle e) const;

    // --------------------------------------------------------------------------------
    // Mutations (scene-wide)
    // --------------------------------------------------------------------------------
    void clearScene(const std::string& mutationGroup = "");
    void restoreScene(const SceneDTO& dto, const std::string& mutationGroup = "");

    // --------------------------------------------------------------------------------
    // Mutations (game objects)
    // --------------------------------------------------------------------------------
    EntityHandle createObject(const std::string& mutationGroup = "");
    void destroyObject(EntityHandle e, const std::string& mutationGroup = "");
    void restoreObject(const GameObjectDTO& dto, const std::string& mutationGroup = "");

private:
    void restore_Impl(const SceneDTO& dto);
    void restore_Impl(const GameObjectDTO& dto);

private:
    EntityManager& store_;
    UndoHistory& undo_;
};
