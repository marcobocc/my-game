#pragma once
#include <functional>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "systems/assets/AssetManager.hpp"
#include "systems/core/GameWindow.hpp"
#include "systems/scene/GameObject.hpp"
#include "systems/scene/Scene.hpp"
#include "systems/ui/UserInterface.hpp"

struct Renderer;
struct Transform;
struct RendererSettings;
class InputSystem;
class PickingSystem;
class PhysicsSystem;
class RenderSystem;
class TimeManager;
class VulkanGraphicsBackend;

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
                        UserInterface& userInterface,
                        AssetManager& assetManager,
                        InputSystem& inputSystem,
                        PickingSystem& pickingSystem,
                        PhysicsSystem& physicsSystem,
                        Scene& scene,
                        RenderSystem& renderSystem,
                        RendererSettings& rendererSettings,
                        VulkanGraphicsBackend& graphicsBackend);

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

    void requestPick(uint32_t x, uint32_t y) const;
    std::optional<std::string> getPickResult() const;

    // --------------------------------------------------------
    // Rendering API
    // --------------------------------------------------------
    void enableWorldGrid();
    void disableWorldGrid();
    void toggleWorldGrid();

    void enableLighting();
    void disableLighting();
    void toggleLighting();

    void drawObjectOutline(const Renderer& renderer, const Transform& transform, std::string objectId) const;
    void drawGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const;

    template<typename T, typename... Args>
    T* emplaceWidget(Args&&... args) {
        return userInterface_.emplace<T>(std::forward<Args>(args)...);
    }

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

    std::string createCamera(const Scene::_createCamera_Options& options);
    std::string createCube(const Scene::_createMesh_Options& options);
    std::string createRectangle2D(const Scene::_createMesh_Options& options);
    std::string createMesh(const std::string& meshName, const Scene::_createMesh_Options& options);
    std::string createModel(const std::string& modelName, const Scene::_createModel_Options& options);

    nlohmann::json serializeScene() const;
    void deserializeScene(const nlohmann::json& j);

private:
    bool shouldClose() const;

    GameWindow& window_;
    TimeManager& time_;
    UserInterface& userInterface_;
    AssetManager& assetManager_;
    InputSystem& inputSystem_;
    PickingSystem& pickingSystem_;
    PhysicsSystem& physicsSystem_;
    Scene& scene_;
    RenderSystem& renderSystem_;
    RendererSettings& rendererSettings_;
    VulkanGraphicsBackend& graphicsBackend_;
};
