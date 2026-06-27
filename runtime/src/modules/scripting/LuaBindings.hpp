#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <sol/sol.hpp>
#include "LuaInput.hpp"
#include "LuaWorld.hpp"
#include "modules/scene/components/Animator.hpp"
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

        // ---- EntityHandle ------------------------------------------------------

        lua.new_usertype<EntityHandle>("EntityHandle");

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

        // ---- World facade ------------------------------------------------------

        lua.new_usertype<LuaWorld>("LuaWorld",
                                   "getTransform",
                                   &LuaWorld::getTransform,
                                   "setTransform",
                                   &LuaWorld::setTransform,
                                   "setParent",
                                   &LuaWorld::setParent,
                                   "clearParent",
                                   &LuaWorld::clearParent,
                                   "findByName",
                                   &LuaWorld::findByName,
                                   "getScript",
                                   &LuaWorld::getScript,
                                   "getAnimator",
                                   &LuaWorld::getAnimator,
                                   "resolveCollisions",
                                   &LuaWorld::resolveCollisions);

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
    }

} // namespace LuaBindings
