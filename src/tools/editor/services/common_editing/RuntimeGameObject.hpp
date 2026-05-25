#pragma once
#include "../../../../runtime/modules/scene/EntityStore.hpp"
#include "../UndoHistory.hpp"

/*
RuntimeGameObject

A live facade over a single ECS entity inside the EntityStore.

It provides component-level mutation, undo/redo tracking, and
serialization while directly operating on the active ECS state.

It does not store or cache data. All operations reflect immediately
on the underlying EntityStore.

------------------------------------------------------------
Live Behavior
------------------------------------------------------------

- Component operations immediately affect the ECS
- No deferred or staged modifications
- All reads/writes reflect current store state
- Snapshots capture live component state at call time

The object is a thin handle-bound view into the EntityStore.

------------------------------------------------------------
Undo/Redo Integration
------------------------------------------------------------

All component mutations are recorded in UndoHistory.

Each operation stores:
- undo action (restore previous component state)
- redo action (reapply updated state)
- optional group identifier for batching related edits

Undo/redo directly mutates the live ECS state.

------------------------------------------------------------
Component Operations
------------------------------------------------------------

- addComponent
  Inserts or replaces a component in the ECS
  Records undo/redo for removal/restoration

- removeComponent
  Removes a component after backing up its state
  Undo restores the previous component

- mutateComponent
  Applies a user-provided mutation function to a component
  Stores both pre- and post-mutation states for undo/redo

------------------------------------------------------------
Snapshot System
------------------------------------------------------------

Serializes the entity into a GameObjectDTO

Includes:
- entity handle
- all attached components
- full component state captured from the live ECS

The snapshot reflects the current state of the entity at the moment
of invocation.
*/
class RuntimeGameObject {
public:
    RuntimeGameObject(EntityManager& store, UndoHistory& undo, EntityHandle e) :
        store_(store),
        undo_(undo),
        entity_(e) {}

    // --------------------------------------------------------------------------------
    // Read-only Operations
    // --------------------------------------------------------------------------------

    EntityHandle handle() const { return entity_; }

    template<typename Component>
    const Component* getComponent() const {
        return store_.getComponent<Component>(entity_);
    }

    // --------------------------------------------------------------------------------
    // Mutations
    // --------------------------------------------------------------------------------
    template<typename Component>
    void addComponent(const Component& c, const std::string& mutationGroup = "") {
        store_.upsertComponent<Component>(entity_, c);
        EntityManager* store = &store_;
        EntityHandle entity = entity_;
        undo_.push([store, entity] { store->removeComponent<Component>(entity); },
                   [store, entity, c] { store->upsertComponent<Component>(entity, c); },
                   "Add Component",
                   mutationGroup);
    }

    template<typename Component>
    void removeComponent(const std::string& mutationGroup = "") {
        auto* old = store_.getComponent<Component>(entity_);
        if (!old) return;
        Component backup = *old;
        store_.removeComponent<Component>(entity_);
        EntityManager* store = &store_;
        EntityHandle entity = entity_;
        undo_.push([store, entity, backup] { store->upsertComponent<Component>(entity, backup); },
                   [store, entity] { store->removeComponent<Component>(entity); },
                   "Remove Component",
                   mutationGroup);
    }

    template<typename Component, typename Fn>
    void mutateComponent(Fn&& fn, const std::string& mutationGroup = "") {
        auto* c = store_.getComponent<Component>(entity_);
        if (!c) return;
        Component before = *c;
        fn(*c);
        Component after = *c;
        EntityManager* store = &store_;
        EntityHandle entity = entity_;
        undo_.push([store, entity, before] { store->upsertComponent<Component>(entity, before); },
                   [store, entity, after] { store->upsertComponent<Component>(entity, after); },
                   "Modify Component",
                   mutationGroup);
    }

private:
    EntityManager& store_;
    UndoHistory& undo_;
    EntityHandle entity_;
};
