#pragma once
#include "../UndoHistory.hpp"
#include "RuntimeGameObject.hpp"
#include "modules/scene/World.hpp"
#include "transport/GameObjectDTO.hpp"
#include "transport/SceneDTO.hpp"

/*
RuntimeScene

A live facade over the World, responsible for entity lifecycle
management, serialization, and undo/redo integration.

It does not own data. Instead, it directly reflects and manipulates the
current state of the underlying World in real time.

Any operation performed through this class immediately affects the live scene.

------------------------------------------------------------
Live Behavior
------------------------------------------------------------

Because this is a live facade:

- There is no staging or buffering of changes
- Entity creation/destruction is immediate
- Component modifications affect the active scene instantly
- Snapshots reflect the current World state at the moment of call

The facade always mirrors the World one-to-one.

------------------------------------------------------------
Undo/Redo Integration
------------------------------------------------------------

All mutations performed through this facade are recorded in UndoHistory.

Each operation stores:
- undo action (revert World state)
- redo action (reapply World state)
- optional group identifier for batching

Undo operations directly mutate the live World.

------------------------------------------------------------
Snapshot System
------------------------------------------------------------

The scene can be serialized into a SceneDTO representing the full
live World state at the time of capture.

Snapshots include:
- all active entities
- all attached components per entity
- full component state
*/
class RuntimeScene {
public:
    RuntimeScene(World& world, UndoHistory& undo) : world_(world), undo_(undo) {}

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
    World& world_;
    UndoHistory& undo_;
    EntityHandle nextHandle_ = 0;
};
