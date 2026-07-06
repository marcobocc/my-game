#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <sol/sol.hpp>
#include "LuaInput.hpp"
#include "LuaWorld.hpp"
#include "modules/scene/components/Animator.hpp"
#include "modules/scene/components/TextComponent.hpp"
#include "modules/scene/components/Transform.hpp"

namespace LuaBindings {

    inline void registerAll(sol::state& lua, LuaWorld& world, LuaInput& input) {
        // ---- Math types --------------------------------------------------------

        lua.new_usertype<glm::vec3>(
                "Vec3",
                sol::call_constructor,
                sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
                "x",
                &glm::vec3::x,
                "y",
                &glm::vec3::y,
                "z",
                &glm::vec3::z,
                sol::meta_function::addition,
                [](const glm::vec3& a, const glm::vec3& b) { return a + b; },
                sol::meta_function::subtraction,
                [](const glm::vec3& a, const glm::vec3& b) { return a - b; },
                sol::meta_function::multiplication,
                sol::overload([](const glm::vec3& a, float s) { return a * s; },
                              [](float s, const glm::vec3& a) { return a * s; }),
                "length",
                [](const glm::vec3& v) { return glm::length(v); },
                "normalize",
                [](const glm::vec3& v) { return glm::normalize(v); },
                "dot",
                [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
                "cross",
                [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); });

        lua.new_usertype<glm::quat>("Quat",
                                    sol::call_constructor,
                                    sol::constructors<glm::quat(), glm::quat(float, float, float, float)>(),
                                    "x",
                                    &glm::quat::x,
                                    "y",
                                    &glm::quat::y,
                                    "z",
                                    &glm::quat::z,
                                    "w",
                                    &glm::quat::w,
                                    sol::meta_function::multiplication,
                                    sol::overload([](const glm::quat& a, const glm::quat& b) { return a * b; },
                                                  [](const glm::quat& q, const glm::vec3& v) { return q * v; }));

        // Helper: build a quaternion from axis-angle (degrees)
        lua["quatAxisAngle"] = [](const glm::vec3& axis, float degrees) {
            return glm::angleAxis(glm::radians(degrees), glm::normalize(axis));
        };

        // ---- Transform ---------------------------------------------------------

        lua.new_usertype<Transform>("Transform",
                                    sol::call_constructor,
                                    sol::constructors<Transform()>(),
                                    "position",
                                    &Transform::position,
                                    "rotation",
                                    &Transform::rotation,
                                    "scale",
                                    &Transform::scale,
                                    "getForward",
                                    &Transform::getForward,
                                    "getRight",
                                    &Transform::getRight,
                                    "getUp",
                                    &Transform::getUp);

        // ---- Entity ------------------------------------------------------------
        // Opaque handle wrapper. Scripts operate on entities via methods on this
        // usertype and never touch the raw uint64_t handle (which cannot round-trip
        // through Lua's double-based number type without losing precision).

        lua.new_usertype<LuaEntity>("Entity",
                                    "isValid",
                                    &LuaEntity::isValid,
                                    "getTransform",
                                    &LuaEntity::getTransform,
                                    "setTransform",
                                    &LuaEntity::setTransform,
                                    "setParent",
                                    &LuaEntity::setParent,
                                    "clearParent",
                                    &LuaEntity::clearParent,
                                    "destroy",
                                    &LuaEntity::destroy,
                                    "getAnimator",
                                    &LuaEntity::getAnimator,
                                    "resolveCollisions",
                                    &LuaEntity::resolveCollisions,
                                    "getScript",
                                    &LuaEntity::getScript,
                                    "getTextComponent",
                                    &LuaEntity::getTextComponent,
                                    "setTextComponent",
                                    &LuaEntity::setTextComponent,
                                    "addTextComponent",
                                    &LuaEntity::addTextComponent,
                                    "removeTextComponent",
                                    &LuaEntity::removeTextComponent,
                                    sol::meta_function::equal_to,
                                    &LuaEntity::operator==,
                                    sol::meta_function::to_string,
                                    &LuaEntity::toString);

        // ---- Animator ----------------------------------------------------------

        lua.new_usertype<Animator>(
                "Animator",
                "play",
                &Animator::play,
                "pause",
                &Animator::pause,
                "stop",
                &Animator::stop,
                "seek",
                [](Animator& a, float t) { a.currentTime = t; },
                "currentState",
                &Animator::currentState,
                "currentTime",
                &Animator::currentTime,
                "playing",
                &Animator::playing,
                "clipNames",
                &Animator::clipNames);

        // ---- TextComponent -----------------------------------------------------

        lua.new_usertype<TextComponent>(
                "TextComponent",
                sol::call_constructor,
                sol::constructors<TextComponent()>(),
                "text",
                &TextComponent::text,
                "fontName",
                &TextComponent::fontName,
                "fontSize",
                &TextComponent::fontSize,
                "visible",
                &TextComponent::visible,
                "billboard",
                &TextComponent::billboard,
                "r",
                sol::property([](const TextComponent& t) { return t.color.r; },
                              [](TextComponent& t, float v) { t.color.r = v; }),
                "g",
                sol::property([](const TextComponent& t) { return t.color.g; },
                              [](TextComponent& t, float v) { t.color.g = v; }),
                "b",
                sol::property([](const TextComponent& t) { return t.color.b; },
                              [](TextComponent& t, float v) { t.color.b = v; }),
                "a",
                sol::property([](const TextComponent& t) { return t.color.a; },
                              [](TextComponent& t, float v) { t.color.a = v; }),
                "alignment",
                sol::property([](const TextComponent& t) { return static_cast<int>(t.alignment); },
                              [](TextComponent& t, int v) { t.alignment = static_cast<TextAlignment>(v); }));

        // ---- World facade ------------------------------------------------------

        // World-level surface only. Per-entity operations live on the Entity usertype;
        // World returns Entities from these lookups rather than exposing raw handles.
        lua.new_usertype<LuaWorld>(
                "LuaWorld", "createEntity", &LuaWorld::createEntity, "findByName", &LuaWorld::findByName);

        // ---- Input facade ------------------------------------------------------

        lua.new_usertype<LuaInput>("LuaInput",
                                   "isKeyDown",
                                   &LuaInput::isKeyDown,
                                   "isKeyPressed",
                                   &LuaInput::isKeyPressed,
                                   "isMouseButtonDown",
                                   &LuaInput::isMouseButtonDown,
                                   "getMouseDelta",
                                   &LuaInput::getMouseDelta,
                                   "getMousePosition",
                                   &LuaInput::getMousePosition,
                                   "getScrollDelta",
                                   &LuaInput::getScrollDelta,
                                   "lockMouse",
                                   &LuaInput::lockMouse,
                                   "unlockMouse",
                                   &LuaInput::unlockMouse);

        // ---- Globals -----------------------------------------------------------

        lua["World"] = &world;
        lua["Input"] = &input;

        lua["TextAlign"] = lua.create_table_with("Left",
                                                 static_cast<int>(TextAlignment::Left),
                                                 "Center",
                                                 static_cast<int>(TextAlignment::Center),
                                                 "Right",
                                                 static_cast<int>(TextAlignment::Right));
    }

} // namespace LuaBindings
