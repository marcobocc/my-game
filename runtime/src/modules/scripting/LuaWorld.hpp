#pragma once
#include <functional>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include "modules/physics/collision_utils.hpp"
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/TransformUtils.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/Animator.hpp"
#include "modules/scene/components/BoxCollider.hpp"
#include "modules/scene/components/CapsuleCollider.hpp"
#include "modules/scene/components/Transform.hpp"

// Thin facade over World exposed to Lua scripts.
// Returns copies of components to keep ownership simple.
class LuaWorld {
public:
    using ScriptLookup = std::function<std::optional<sol::table>(EntityHandle, const std::string&)>;

    explicit LuaWorld(World& world) : world_(world) {}

    void setScriptLookup(ScriptLookup fn) { scriptLookup_ = std::move(fn); }

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

    Animator* getAnimator(EntityHandle entity) const {
        Actor* actor = world_.getActor(entity);
        if (!actor) return nullptr;
        return actor->getComponent<Animator>();
    }

    // Returns the accumulated MTV to push entity out of all overlapping box colliders.
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

    // Finds the first actor whose name tag matches. Returns invalid handle if not found.
    // (Name lookup requires a NameTag component — returns nullopt if not present.)
    std::optional<EntityHandle> findByName(const std::string& name) const {
        for (const auto& actor: world_.getActors()) {
            // NameTag component not yet implemented — placeholder for future use
            (void) name;
            (void) actor;
        }
        return std::nullopt;
    }

private:
    World& world_;
    ScriptLookup scriptLookup_;
};
