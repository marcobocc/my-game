#pragma once
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>
#include "modules/assets/AssetManager.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/scene/EntityManager.hpp"

struct AABB;
struct Camera;
struct Renderer;
struct Transform;
struct RendererSettings;
class InputSystem;
class PhysicsSystem;
class GameRenderSystem;
class TimeManager;
class VulkanGameRenderer;

/*
    GameEngine (Facade)

    Purpose:
    --------------------------------------------------
    High-level facade and API for the game engine

    Responsibilities:
    --------------------------------------------------
    - Provide the API for all game engine operations
    - Forward calls to underlying engine systems
    - Hide internal engine complexity and modularity
      from the application layer

    This class is not responsible for:
    --------------------------------------------------
    - Creating or owning engine subsystems
    - Subsystem wiring or composition
    - Managing dependencies lifetime
*/
class GameEngine {
public:
    explicit GameEngine(GameWindow& window,
                        TimeManager& time,
                        AssetManager& assetManager,
                        InputSystem& inputSystem,
                        PhysicsSystem& physicsSystem,
                        EntityManager& entityManager,
                        GameRenderSystem& renderSystem,
                        RendererSettings& rendererSettings,
                        VulkanGameRenderer& renderer);

    // --------------------------------------------------------
    // Game Loop
    // --------------------------------------------------------
    void run(const std::function<void(double deltaTime)>& gameLoopFunc);
    void requestClose() const;

    // --------------------------------------------------------
    // Input API
    // --------------------------------------------------------
    std::pair<double, double> getMousePosition() const;
    double getScrollDelta() const;

    bool isKeyDown(int key) const;
    bool isKeyPressed(int key) const;
    bool isMouseButtonDown(int button) const;

    // --------------------------------------------------------
    // Rendering API
    // --------------------------------------------------------
    void setActiveCamera(const Camera& camera, const Transform& transform);


    // --------------------------------------------------------
    // Assets API
    // --------------------------------------------------------
    std::vector<std::string> getAvailableAssets(const std::string& extension) const;
    template<typename T>
    T* getAsset(const std::string& name) const {
        return assetManager_.get<T>(name);
    }

    // --------------------------------------------------------
    // Entity API
    // --------------------------------------------------------
    EntityManager& entities() { return entityManager_; }

private:
    bool shouldClose() const;

    GameWindow& window_;
    TimeManager& time_;
    AssetManager& assetManager_;
    InputSystem& inputSystem_;
    PhysicsSystem& physicsSystem_;
    EntityManager& entityManager_;
    GameRenderSystem& renderSystem_;
    RendererSettings& rendererSettings_;
    VulkanGameRenderer& renderer_;
};
