#pragma once
#include <functional>
#include <typeindex>

class World;
class InputSystem;

struct ScriptInstanceContext {
    World* world = nullptr;
    InputSystem* inputSystem = nullptr;
    // Type-erased lookup: finds a live script instance of the given type for the given entity.
    std::function<void*(std::type_index, unsigned int)> findScriptInstance;
};
