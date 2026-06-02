#pragma once

class World;
class InputSystem;

struct ScriptInstanceContext {
    World* world = nullptr;
    InputSystem* inputSystem = nullptr;
};
