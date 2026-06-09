#pragma once
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include "modules/asset_management/AssetCache.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/physics/PhysicsSystem.hpp"
#include "modules/scene/World.hpp"
#include "modules/scene/components/gameplay/BehaviourComponent.hpp"

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
                 InputSystem& inputSystem,
                 World world,
                 GameRenderSystem& renderSystem,
                 RendererSettings& rendererSettings,
                 IGameRenderer& renderer);

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

private:
    bool shouldClose() const;
    void spawnBehaviours();

    GameWindow& window_;
    TimeManager& time_;
    AssetCache& loadedAssets_;
    InputSystem& inputSystem_;
    World world_;
    PhysicsSystem physicsSystem_;
    GameRenderSystem& renderSystem_;
    RendererSettings& rendererSettings_;
    IGameRenderer& renderer_;

    DeveloperConsole developerConsole_;
    std::vector<BehaviourComponent*> behaviours_;
};
