#include "GameEngine.hpp"
#include "data/components/Camera.hpp"
#include "data/components/Transform.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/core/TimeManager.hpp"
#include "systems/input/InputSystem.hpp"
#include "systems/physics/PhysicsSystem.hpp"
#include "systems/rendering/GameRenderSystem.hpp"

GameEngine::GameEngine(GameWindow& window,
                       TimeManager& time,
                       AssetManager& assetManager,
                       InputSystem& inputSystem,
                       PhysicsSystem& physicsSystem,
                       SceneManager& scene,
                       GameRenderSystem& renderSystem,
                       RendererSettings& rendererSettings,
                       VulkanGameRenderer& renderer) :
    window_(window),
    time_(time),
    assetManager_(assetManager),
    inputSystem_(inputSystem),
    physicsSystem_(physicsSystem),
    sceneManager_(scene),
    renderSystem_(renderSystem),
    rendererSettings_(rendererSettings),
    renderer_(renderer) {}

// --------------------------------------------------------
// Game Loop
// --------------------------------------------------------

void GameEngine::run(const std::function<void(double deltaTime)>& gameLoopFunc) {
    while (!shouldClose()) {
        time_.beginFrame();
        float deltaTime = time_.getGameDeltaTime();

        window_.pollEvents();
        inputSystem_.update();
        physicsSystem_.update();

        gameLoopFunc(deltaTime);
        renderSystem_.update(sceneManager_);
        time_.endFrame();
    }
}

bool GameEngine::shouldClose() const { return window_.shouldClose(); }

void GameEngine::requestClose() const { window_.requestClose(); }

// --------------------------------------------------------
// Input API
// --------------------------------------------------------

std::pair<double, double> GameEngine::getMousePosition() const { return inputSystem_.getMousePosition(); }

bool GameEngine::isKeyDown(int key) const { return inputSystem_.isKeyDown(key); }

bool GameEngine::isKeyPressed(int key) const { return inputSystem_.isKeyPressed(key); }

bool GameEngine::isMouseButtonDown(int button) const { return inputSystem_.isMouseButtonDown(button); }

double GameEngine::getScrollDelta() const { return inputSystem_.getScrollDelta(); }

// --------------------------------------------------------
// Rendering API
// --------------------------------------------------------

void GameEngine::setActiveCamera(const Camera& camera, const Transform& transform) {
    renderSystem_.setActiveCamera(camera, transform);
}

// --------------------------------------------------------
// Assets API
// --------------------------------------------------------

std::pair<std::string, bool> GameEngine::createEmptyObject(const std::string& name) {
    return sceneManager_.createEmptyObject(name);
}

std::string GameEngine::createCamera(const SceneManager::_createCamera_Options& options) {
    return sceneManager_.createCamera(options);
}

std::string GameEngine::createCube(const SceneManager::_createMesh_Options& options) {
    return sceneManager_.createCube(options);
}

std::string GameEngine::createRectangle2D(const SceneManager::_createMesh_Options& options) {
    return sceneManager_.createRectangle2D(options);
}

std::string GameEngine::createMesh(const std::string& meshName, const SceneManager::_createMesh_Options& options) {
    return sceneManager_.createMesh(meshName, options);
}

std::string GameEngine::createModel(const std::string& modelName, const SceneManager::_createModel_Options& options) {
    return sceneManager_.createModel(modelName, options);
}

void GameEngine::destroyObject(const std::string& name) { sceneManager_.destroyObject(name); }

GameObject& GameEngine::getObject(const std::string& name) const { return sceneManager_.getObject(name); }

const std::unordered_map<std::string, GameObject>& GameEngine::getObjects() const { return sceneManager_.getObjects(); }

void GameEngine::loadScene(SceneManager& scene, const nlohmann::json& j) { SceneManager::deserialize(j, scene); }

// --------------------------------------------------------
// Assets API
// --------------------------------------------------------

std::vector<std::string> GameEngine::getAvailableAssets(const std::string& extension) const {
    return assetManager_.getAvailableAssets(extension);
}
