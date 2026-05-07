#pragma once
#include <functional>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "modules/assets/AssetManager.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/scene/GameObject.hpp"
#include "modules/scene/Scene.hpp"

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
                        Scene& scene,
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
    // Scene API
    // --------------------------------------------------------
    GameObject& getObject(const std::string& name) const;
    const std::unordered_map<std::string, GameObject>& getObjects() const;
    void destroyObject(const std::string& name);

    std::pair<std::string, bool> createEmptyObject(const std::string& name = "");
    std::string createCamera(const Scene::_createCamera_Options& options);
    std::string createCube(const Scene::_createMesh_Options& options);
    std::string createRectangle2D(const Scene::_createMesh_Options& options);
    std::string createMesh(const std::string& meshName, const Scene::_createMesh_Options& options);
    std::string createModel(const std::string& modelName, const Scene::_createModel_Options& options);
    void loadScene(Scene& scene, const nlohmann::json& j);

private:
    bool shouldClose() const;

    GameWindow& window_;
    TimeManager& time_;
    AssetManager& assetManager_;
    InputSystem& inputSystem_;
    PhysicsSystem& physicsSystem_;
    Scene& scene_;
    GameRenderSystem& renderSystem_;
    RendererSettings& rendererSettings_;
    VulkanGameRenderer& renderer_;
};
