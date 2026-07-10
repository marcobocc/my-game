#pragma once
#include <optional>
#include <sol/sol.hpp>
#include <string>
#include "../../animation/components/Animator.hpp"
#include "../../core/components/Transform.hpp"
#include "../../graphics/components/TextComponent.hpp"
#include "core/scene/EntityHandle.hpp"

class LuaWorld;

// Opaque handle wrapper exposed to Lua. Scripts never see the raw uint64_t
// handle (which would lose precision as a Lua double); they receive an Entity
// and call operations as methods on it. An invalid Entity (isValid() == false)
// stands in for "no entity" — use it instead of comparing handles against 0.
class LuaEntity {
public:
    LuaEntity() = default;
    LuaEntity(LuaWorld* world, EntityHandle handle) : world_(world), handle_(handle) {}

    EntityHandle handle() const { return handle_; }
    bool isValid() const { return world_ != nullptr && handle_ != INVALID_ENTITY_HANDLE; }

    bool operator==(const LuaEntity& other) const { return handle_ == other.handle_; }

    std::string toString() const { return "Entity(" + std::to_string(handle_) + ")"; }

    // --- Per-entity operations (defined in LuaWorld.hpp, which has the full LuaWorld type) ---
    std::optional<Transform> getTransform() const;
    void setTransform(const Transform& t);
    void setParent(const LuaEntity& parent);
    void clearParent();
    void destroy();
    Animator* getAnimator() const;
    glm::vec3 resolveCollisions() const;
    std::optional<sol::table> getScript(const std::string& scriptName) const;
    std::optional<TextComponent> getTextComponent() const;
    void setTextComponent(const TextComponent& component);
    void addTextComponent(const TextComponent& component);
    void removeTextComponent();

private:
    LuaWorld* world_ = nullptr;
    EntityHandle handle_ = INVALID_ENTITY_HANDLE;
};
