#pragma once
#include <functional>
#include <string>
#include <utility>
#include "GameWindow.hpp"
#include "animation/AnimationSystem.hpp"
#include "console/DeveloperConsole.hpp"
#include "core/assets/AssetCache.hpp"
#include "core/assets/AssetLoader.hpp"
#include "graphics/debug/DebugDraw.hpp"
#include "physics/PhysicsSystem.hpp"
#include "scene/World.hpp"
#include "scripting/LuaScriptSystem.hpp"
#include "ui/UISystem.hpp"

struct AABB;
struct Camera;
struct Renderer;
struct Transform;
struct RendererSettings;
class InputSystem;
class GameRenderSystem;
class TimeManager;

/*
    GameInstance

    Purpose:
    --------------------------------------------------
    Owns and runs an isolated game simulation from a World snapshot.
    Created at simulation start, destroyed at simulation stop.

    This class is not responsible for:
    --------------------------------------------------
    - The editor's World — it operates on its own copy
    - Subsystem wiring or composition
*/
class GameInstance {
public:
    GameInstance(GameWindow& window,
                 TimeManager& time,
                 AssetCache& loadedAssets,
                 AssetLoader& assetLoader,
                 InputSystem& inputSystem,
                 World world,
                 GameRenderSystem& renderSystem);

    ~GameInstance();

    // --------------------------------------------------------
    // Game Loop
    // --------------------------------------------------------
    void tick(float deltaTime);
    void run(const std::function<void(double deltaTime)>& gameLoopFunc);
    void requestClose() const;

    // --------------------------------------------------------
    // Input API
    // --------------------------------------------------------
    std::pair<double, double> getMousePosition() const;
    std::pair<double, double> getMouseDelta() const;
    double getScrollDelta() const;

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool isMouseButtonDown(int button) const;

    void lockMouse() const;
    void unlockMouse() const;

    // --------------------------------------------------------
    // Rendering API
    // --------------------------------------------------------
    void setActiveCamera(const Camera& camera, const Transform& transform);

    // --------------------------------------------------------
    // Assets API
    // --------------------------------------------------------
    template<typename T>
    T* getAsset(const std::string& name) const {
        return loadedAssets_.get<T>(name);
    }

    // --------------------------------------------------------
    // Entity API
    // --------------------------------------------------------
    World& entities() { return world_; }
    DeveloperConsole& developerConsole() { return developerConsole_; }
    PhysicsSystem& physicsSystem() { return physicsSystem_; }
    AnimationSystem& animationSystem() { return animationSystem_; }
    DebugDraw& debugDraw() { return debugDraw_; }
    UISystem& uiSystem() { return uiSystem_; }

private:
    bool shouldClose() const;

    GameWindow& window_;
    TimeManager& time_;
    AssetCache& loadedAssets_;
    AssetLoader& assetLoader_;
    InputSystem& inputSystem_;
    World world_;
    PhysicsSystem physicsSystem_;
    GameRenderSystem& renderSystem_;

    DeveloperConsole developerConsole_;
    AnimationSystem animationSystem_;
    DebugDraw debugDraw_;
    LuaScriptSystem luaScriptSystem_;
    // Declared after luaScriptSystem_ so widgets (which hold Lua callbacks)
    // are destroyed before the Lua state.
    UISystem uiSystem_;
};
