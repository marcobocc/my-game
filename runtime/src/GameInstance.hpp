#pragma once
#include <functional>
#include <string>
#include <utility>
#include "modules/animation/AnimationSystem.hpp"
#include "modules/asset_management/AssetCache.hpp"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/debug/DebugDraw.hpp"
#include "modules/debug/console/DeveloperConsole.hpp"
#include "modules/physics/PhysicsSystem.hpp"
#include "modules/scene/World.hpp"
#include "modules/scripting/LuaScriptSystem.hpp"

struct AABB;
struct Camera;
struct Renderer;
struct Transform;
struct RendererSettings;
class InputSystem;
class GameRenderSystem;
class TimeManager;
#include "modules/rendering/IGameRenderer.hpp"

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
                 GameRenderSystem& renderSystem,
                 RendererSettings& rendererSettings,
                 IGameRenderer& renderer);

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
    RendererSettings& rendererSettings_;
    IGameRenderer& renderer_;

    DeveloperConsole developerConsole_;
    AnimationSystem animationSystem_;
    DebugDraw debugDraw_;
    LuaScriptSystem luaScriptSystem_;
};
