#pragma once
#include <functional>
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include "modules/scene/EntityHandle.hpp"
#include "modules/scene/World.hpp"
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
