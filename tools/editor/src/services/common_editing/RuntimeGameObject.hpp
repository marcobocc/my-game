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
  Inserts a component on the Actor. For single-instance types,
  replaces the existing one. For multi-instance types (allowMultiple),
  always appends a new instance.
  Records undo/redo for removal/restoration.

- removeComponent / removeComponentAt
  Removes a component after backing up its state.
  removeComponentAt targets a specific instance by index.
  Undo restores the previous component.

- mutateComponent / mutateComponentAt
  Applies a user-provided mutation function to a component.
  mutateComponentAt targets a specific instance by index.
  Stores both pre- and post-mutation states for undo/redo.

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

    template<typename Component>
    std::vector<Component*> getComponents() {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return {};
        return actor->getComponents<Component>();
    }

    // --------------------------------------------------------------------------------
    // Mutations
    // --------------------------------------------------------------------------------

    template<typename Component>
    void addComponent(const Component& c, const std::string& mutationGroup = "") {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return;
        Component* existing = actor->template getComponent<Component>();
        if (existing && !c.allowMultiple())
            *existing = c;
        else
            actor->template addComponent<Component>(c);
        World* world = &world_;
        EntityHandle entity = entity_;
        undo_.push(
                [world, entity] {
                    if (Actor* a = world->getActor(entity)) {
                        auto all = a->template getComponents<Component>();
                        if (!all.empty()) a->removeComponent(all.back());
                    }
                },
                [world, entity, c] {
                    if (Actor* a = world->getActor(entity)) {
                        Component* ex = a->template getComponent<Component>();
                        if (ex && !c.allowMultiple())
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
        removeComponentAt<Component>(0, mutationGroup);
    }

    template<typename Component>
    void removeComponentAt(size_t index, const std::string& mutationGroup = "") {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return;
        auto all = actor->template getComponents<Component>();
        if (index >= all.size()) return;
        Component backup = *all[index];
        actor->removeComponent(all[index]);
        World* world = &world_;
        EntityHandle entity = entity_;
        undo_.push(
                [world, entity, backup] {
                    if (Actor* a = world->getActor(entity)) a->template addComponent<Component>(backup);
                },
                [world, entity, index] {
                    if (Actor* a = world->getActor(entity)) {
                        auto all = a->template getComponents<Component>();
                        if (index < all.size()) a->removeComponent(all[index]);
                    }
                },
                "Remove Component",
                mutationGroup);
    }

    template<typename Component, typename Fn>
    void mutateComponent(Fn&& fn, const std::string& mutationGroup = "") {
        mutateComponentAt<Component>(0, std::forward<Fn>(fn), mutationGroup);
    }

    template<typename Component, typename Fn>
    void mutateComponentAt(size_t index, Fn&& fn, const std::string& mutationGroup = "") {
        Actor* actor = world_.getActor(entity_);
        if (!actor) return;
        auto all = actor->template getComponents<Component>();
        if (index >= all.size()) return;
        Component* c = all[index];
        Component before = *c;
        std::forward<Fn>(fn)(*c);
        Component after = *c;
        World* world = &world_;
        EntityHandle entity = entity_;
        undo_.push(
                [world, entity, index, before] {
                    if (Actor* a = world->getActor(entity)) {
                        auto all = a->template getComponents<Component>();
                        if (index < all.size()) *all[index] = before;
                    }
                },
                [world, entity, index, after] {
                    if (Actor* a = world->getActor(entity)) {
                        auto all = a->template getComponents<Component>();
                        if (index < all.size()) *all[index] = after;
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
