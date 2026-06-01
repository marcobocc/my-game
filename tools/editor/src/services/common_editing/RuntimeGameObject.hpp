#pragma once
#include "../UndoHistory.hpp"
#include "modules/scene/World.hpp"

/*
RuntimeGameObject

A live facade over a single Actor inside the World.

It provides component-level mutation, undo/redo tracking, and
serialization while directly operating on the active scene state.

It does not store or cache data. All operations reflect immediately
on the underlying World.

------------------------------------------------------------
Live Behavior
------------------------------------------------------------

- Component operations immediately affect the Actor
- No deferred or staged modifications
- All reads/writes reflect current World state
- Snapshots capture live component state at call time

The object is a thin handle-bound view into the World.

------------------------------------------------------------
Undo/Redo Integration
------------------------------------------------------------

All component mutations are recorded in UndoHistory.

Each operation stores:
- undo action (restore previous component state)
- redo action (reapply updated state)
- optional group identifier for batching related edits

Undo/redo directly mutates the live World state.

------------------------------------------------------------
Component Operations
------------------------------------------------------------

- addComponent
  Inserts or replaces a component on the Actor
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
- full component state captured from the live World
*/
class RuntimeGameObject {
public:
    RuntimeGameObject(World& world, UndoHistory& undo, EntityHandle e) : world_(world), undo_(undo), entity_(e) {}

    // --------------------------------------------------------------------------------
    // Read-only Operations
    // --------------------------------------------------------------------------------

    EntityHandle handle() const { return entity_; }

    template<typename Component>
    const Component* getComponent() const {
        const Actor* actor = world_.getActor(entity_);
        if (!actor) return nullptr;
        return actor->getComponent<Component>();
    }

    // --------------------------------------------------------------------------------
    // Mutations
    // --------------------------------------------------------------------------------
    template<typename Component>
    void addComponent(const Component& c, const std::string& mutationGroup = "") {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return;
        Component* existing = actor->template getComponent<Component>();
        if (existing)
            *existing = c;
        else
            actor->template addComponent<Component>(c);
        World* world = &world_;
        EntityHandle entity = entity_;
        undo_.push(
                [world, entity] {
                    if (Actor* a = world->getActor(entity)) a->removeComponent(a->template getComponent<Component>());
                },
                [world, entity, c] {
                    if (Actor* a = world->getActor(entity)) {
                        Component* ex = a->template getComponent<Component>();
                        if (ex)
                            *ex = c;
                        else
                            a->template addComponent<Component>(c);
                    }
                },
                "Add Component",
                mutationGroup);
    }

    template<typename Component>
    void removeComponent(const std::string& mutationGroup = "") {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return;
        Component* old = actor->template getComponent<Component>();
        if (!old) return;
        Component backup = *old;
        actor->removeComponent(old);
        World* world = &world_;
        EntityHandle entity = entity_;
        undo_.push(
                [world, entity, backup] {
                    if (Actor* a = world->getActor(entity)) a->template addComponent<Component>(backup);
                },
                [world, entity] {
                    if (Actor* a = world->getActor(entity)) a->removeComponent(a->template getComponent<Component>());
                },
                "Remove Component",
                mutationGroup);
    }

    template<typename Component, typename Fn>
    void mutateComponent(Fn&& fn, const std::string& mutationGroup = "") {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return;
        Component* c = actor->template getComponent<Component>();
        if (!c) return;
        Component before = *c;
        std::forward<Fn>(fn)(*c);
        Component after = *c;
        World* world = &world_;
        EntityHandle entity = entity_;
        undo_.push(
                [world, entity, before] {
                    if (Actor* a = world->getActor(entity)) {
                        if (auto* comp = a->template getComponent<Component>()) *comp = before;
                    }
                },
                [world, entity, after] {
                    if (Actor* a = world->getActor(entity)) {
                        if (auto* comp = a->template getComponent<Component>()) *comp = after;
                    }
                },
                "Modify Component",
                mutationGroup);
    }

private:
    World& world_;
    UndoHistory& undo_;
    EntityHandle entity_;
};
