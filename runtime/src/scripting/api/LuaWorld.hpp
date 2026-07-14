#pragma once
#include <functional>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include "../../animation/components/Animator.hpp"
#include "../../core/components/Transform.hpp"
#include "../../graphics/components/Renderer.hpp"
#include "../../graphics/components/TextComponent.hpp"
#include "../../physics/TerrainHeightfield.hpp"
#include "../../physics/components/BoxCollider.hpp"
#include "../../physics/components/CapsuleCollider.hpp"
#include "../../physics/components/TerrainCollider.hpp"
#include "LuaEntity.hpp"
#include "core/scene/EntityHandle.hpp"
#include "core/scene/TransformUtils.hpp"
#include "core/scene/World.hpp"
#include "physics/CollisionDetection.hpp"

// Thin facade over World exposed to Lua scripts.
// Returns copies of components to keep ownership simple.
//
// Scripts interact with entities through the opaque LuaEntity wrapper (never
// raw handles). The handle-based methods below are the implementation the
// LuaEntity methods delegate to; the Lua-facing surface is Entity methods plus
// the world-level lookups that return Entity (createEntity, findByName).
class LuaWorld {
public:
    using ScriptLookup = std::function<std::optional<sol::table>(EntityHandle, const std::string&)>;
    using MeshLookup = std::function<const Mesh*(const std::string&)>;

    explicit LuaWorld(World& world) : world_(world) {}

    void setScriptLookup(ScriptLookup fn) { scriptLookup_ = std::move(fn); }
    void setMeshLookup(MeshLookup fn) { meshLookup_ = std::move(fn); }

    // Wraps a raw handle into an Entity bound to this world.
    LuaEntity wrap(EntityHandle handle) { return LuaEntity(this, handle); }

    std::optional<sol::table> getScript(EntityHandle entity, const std::string& scriptName) const {
        if (scriptLookup_) return scriptLookup_(entity, scriptName);
        return std::nullopt;
    }

    // Returns a copy of the entity's Transform, or nil if missing.
    std::optional<Transform> getTransform(EntityHandle entity) const {
        const Actor* actor = world_.getActor(entity);
        if (!actor) return std::nullopt;
        const Transform* t = actor->getComponent<Transform>();
        if (!t) return std::nullopt;
        return *t;
    }

    // Writes a Transform back to the entity.
    void setTransform(EntityHandle entity, const Transform& t) {
        Actor* actor = world_.getActor(entity);
        if (!actor) return;
        if (auto* existing = actor->getComponent<Transform>()) *existing = t;
    }

    void setParent(EntityHandle entity, EntityHandle parentEntity) {
        Actor* actor = world_.getActor(entity);
        if (!actor) return;
        if (auto* t = actor->getComponent<Transform>()) t->parent = parentEntity;
    }

    void clearParent(EntityHandle entity) { setParent(entity, INVALID_ENTITY_HANDLE); }

    // Creates a new empty entity with a default Transform and returns it.
    LuaEntity createEntity() {
        EntityHandle handle = world_.generateHandle();
        Actor* actor = world_.addActor(handle);
        if (actor) actor->addComponent<Transform>();
        return wrap(handle);
    }

    void destroyEntity(EntityHandle entity) { world_.removeActor(entity); }

    Animator* getAnimator(EntityHandle entity) const {
        Actor* actor = world_.getActor(entity);
        if (!actor) return nullptr;
        return actor->getComponent<Animator>();
    }

    // Returns the accumulated MTV to push entity out of all overlapping box
    // colliders and terrain heightfields (entities with a TerrainCollider).
    // Supports entities with either a BoxCollider or CapsuleCollider.
    // The entity itself (and its children) are excluded from the check.
    // Returns zero vector if no overlaps.
    glm::vec3 resolveCollisions(EntityHandle entity) const {
        const Actor* actor = world_.getActor(entity);
        if (!actor) return glm::vec3(0.0f);
        const Transform* selfTransform = actor->getComponent<Transform>();
        if (!selfTransform) return glm::vec3(0.0f);

        const BoxCollider* selfBox = actor->getComponent<BoxCollider>();
        const CapsuleCollider* selfCapsule = actor->getComponent<CapsuleCollider>();
        if (!selfBox && !selfCapsule) return glm::vec3(0.0f);

        glm::mat4 selfMat = TransformUtils::resolveWorldMatrix(*selfTransform, world_);
        std::vector<glm::vec3> pushes;

        auto collidables = world_.query<BoxCollider, Transform>();
        for (const auto& [other, otherCollider, otherTransform]: collidables) {
            if (other == entity) continue;
            if (otherTransform->parent == entity) continue;
            glm::mat4 otherMat = TransformUtils::resolveWorldMatrix(*otherTransform, world_);

            std::optional<glm::vec3> mtv;
            if (selfBox)
                mtv = Physics::computeMTV(*selfBox, selfMat, *otherCollider, otherMat);
            else
                mtv = Physics::computeCapsuleBoxMTV(*selfCapsule, selfMat, *otherCollider, otherMat);

            if (mtv) pushes.push_back(*mtv);
        }

        auto terrains = world_.query<TerrainCollider, Transform>();
        if (!terrains.empty()) {
            std::vector<glm::vec3> samplePoints = selfBox ? Physics::terrainSamplePoints(*selfBox, selfMat)
                                                          : Physics::terrainSamplePoints(*selfCapsule, selfMat);
            for (const auto& [other, terrainCollider, terrainTransform]: terrains) {
                if (other == entity) continue;
                const TerrainHeightfield* heightfield = heightfieldFor(other);
                if (!heightfield) continue;
                glm::mat4 terrainMat = TransformUtils::resolveWorldMatrix(*terrainTransform, world_);
                auto mtv = Physics::computeTerrainMTV(samplePoints, *heightfield, terrainMat, glm::inverse(terrainMat));
                if (mtv) pushes.push_back(*mtv);
            }
        }

        if (pushes.empty()) return glm::vec3(0.0f);

        // Find the push with the largest upward component — treat it as the primary ground push.
        // For remaining pushes, only accumulate horizontal (XZ) components to avoid
        // vertical fighting between adjacent colliders at seams.
        int primaryIdx = 0;
        for (int i = 1; i < static_cast<int>(pushes.size()); ++i)
            if (pushes[i].y > pushes[primaryIdx].y) primaryIdx = i;

        glm::vec3 result = pushes[primaryIdx];
        for (int i = 0; i < static_cast<int>(pushes.size()); ++i) {
            if (i == primaryIdx) continue;
            result.x += pushes[i].x;
            result.z += pushes[i].z;
        }
        return result;
    }

    std::optional<TextComponent> getTextComponent(EntityHandle entity) const {
        const Actor* actor = world_.getActor(entity);
        if (!actor) return std::nullopt;
        const TextComponent* c = actor->getComponent<TextComponent>();
        if (!c) return std::nullopt;
        return *c;
    }

    void setTextComponent(EntityHandle entity, const TextComponent& component) {
        Actor* actor = world_.getActor(entity);
        if (!actor) return;
        if (auto* existing = actor->getComponent<TextComponent>()) *existing = component;
    }

    void addTextComponent(EntityHandle entity, const TextComponent& component) {
        Actor* actor = world_.getActor(entity);
        if (!actor) return;
        if (!actor->getComponent<TextComponent>()) actor->addComponent<TextComponent>(component);
    }

    void removeTextComponent(EntityHandle entity) {
        Actor* actor = world_.getActor(entity);
        if (!actor) return;
        if (auto* existing = actor->getComponent<TextComponent>()) actor->removeComponent(existing);
    }

    // Finds the first actor whose name tag matches. Returns an invalid Entity if not found.
    // (Name lookup requires a NameTag component — not yet implemented.)
    LuaEntity findByName(const std::string& name) {
        for (const auto& actor: world_.getActors()) {
            // NameTag component not yet implemented — placeholder for future use
            (void) name;
            (void) actor;
        }
        return LuaEntity(this, INVALID_ENTITY_HANDLE);
    }

private:
    // Heightfields are built lazily from the terrain's Renderer mesh and cached
    // per mesh name; mesh assets don't change while a simulation is running.
    const TerrainHeightfield* heightfieldFor(EntityHandle terrainEntity) const {
        const Actor* actor = world_.getActor(terrainEntity);
        if (!actor || !meshLookup_) return nullptr;
        const Renderer* renderer = actor->getComponent<Renderer>();
        if (!renderer) return nullptr;

        auto it = heightfields_.find(renderer->meshName);
        if (it == heightfields_.end()) {
            const Mesh* mesh = meshLookup_(renderer->meshName);
            std::optional<TerrainHeightfield> built = mesh ? TerrainHeightfield::build(*mesh) : std::nullopt;
            it = heightfields_.emplace(renderer->meshName, std::move(built)).first;
        }
        return it->second ? &*it->second : nullptr;
    }

    World& world_;
    ScriptLookup scriptLookup_;
    MeshLookup meshLookup_;
    mutable std::unordered_map<std::string, std::optional<TerrainHeightfield>> heightfields_;
};

// ---- LuaEntity method definitions -----------------------------------------
// Defined here (after LuaWorld is complete) so each Entity operation delegates
// to the handle-based LuaWorld method.

inline std::optional<Transform> LuaEntity::getTransform() const { return world_->getTransform(handle_); }
inline void LuaEntity::setTransform(const Transform& t) { world_->setTransform(handle_, t); }
inline void LuaEntity::setParent(const LuaEntity& parent) { world_->setParent(handle_, parent.handle_); }
inline void LuaEntity::clearParent() { world_->clearParent(handle_); }
inline void LuaEntity::destroy() { world_->destroyEntity(handle_); }
inline Animator* LuaEntity::getAnimator() const { return world_->getAnimator(handle_); }
inline glm::vec3 LuaEntity::resolveCollisions() const { return world_->resolveCollisions(handle_); }
inline std::optional<sol::table> LuaEntity::getScript(const std::string& scriptName) const {
    return world_->getScript(handle_, scriptName);
}
inline std::optional<TextComponent> LuaEntity::getTextComponent() const { return world_->getTextComponent(handle_); }
inline void LuaEntity::setTextComponent(const TextComponent& c) { world_->setTextComponent(handle_, c); }
inline void LuaEntity::addTextComponent(const TextComponent& c) { world_->addTextComponent(handle_, c); }
inline void LuaEntity::removeTextComponent() { world_->removeTextComponent(handle_); }
