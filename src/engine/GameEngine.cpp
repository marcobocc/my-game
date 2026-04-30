#include "GameEngine.hpp"
#include "data/settings/RendererSettings.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/core/TimeManager.hpp"
#include "systems/input/InputSystem.hpp"
#include "systems/input/PickingSystem.hpp"
#include "systems/physics/PhysicsSystem.hpp"
#include "systems/rendering/RenderSystem.hpp"
#include "systems/rendering/vulkan/VulkanGraphicsBackend.hpp"
#include "systems/scene/SceneSerializer.hpp"

GameEngine::GameEngine(GameWindow& window,
                       TimeManager& time,
                       UserInterface& userInterface,
                       AssetManager& assetManager,
                       InputSystem& inputSystem,
                       PickingSystem& pickingSystem,
                       PhysicsSystem& physicsSystem,
                       Scene& scene,
                       RenderSystem& renderSystem,
                       RendererSettings& rendererSettings,
                       VulkanGraphicsBackend& graphicsBackend) :
    window_(window),
    time_(time),
    userInterface_(userInterface),
    assetManager_(assetManager),
    inputSystem_(inputSystem),
    pickingSystem_(pickingSystem),
    physicsSystem_(physicsSystem),
    scene_(scene),
    renderSystem_(renderSystem),
    rendererSettings_(rendererSettings),
    graphicsBackend_(graphicsBackend) {}

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
        renderSystem_.update(scene_);
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

void GameEngine::requestPick(uint32_t x, uint32_t y) const { pickingSystem_.requestPick(x, y); }

std::optional<std::string> GameEngine::getPickResult() const { return pickingSystem_.getPickResult(); }

// --------------------------------------------------------
// Rendering API
// --------------------------------------------------------

void GameEngine::enableWorldGrid() { rendererSettings_.enableGrid = true; }
void GameEngine::disableWorldGrid() { rendererSettings_.enableGrid = false; }
void GameEngine::toggleWorldGrid() { rendererSettings_.enableGrid = !rendererSettings_.enableGrid; }

void GameEngine::enableLighting() { rendererSettings_.enableLighting = true; }
void GameEngine::disableLighting() { rendererSettings_.enableLighting = false; }
void GameEngine::toggleLighting() { rendererSettings_.enableLighting = !rendererSettings_.enableLighting; }

void GameEngine::drawObjectOutline(const Renderer& renderer, const Transform& transform, std::string objectId) const {
    graphicsBackend_.outline(renderer, transform, std::move(objectId));
}

void GameEngine::drawGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const {
    graphicsBackend_.submitGizmoLine(from, to, color);
}

// --------------------------------------------------------
// Scene API
// --------------------------------------------------------

std::string GameEngine::createCamera(const Scene::_createCamera_Options& options) {
    return scene_.createCamera(options);
}

std::string GameEngine::createCube(const Scene::_createMesh_Options& options) { return scene_.createCube(options); }

std::string GameEngine::createRectangle2D(const Scene::_createMesh_Options& options) {
    return scene_.createRectangle2D(options);
}

std::string GameEngine::createMesh(const std::string& meshName, const Scene::_createMesh_Options& options) {
    return scene_.createMesh(meshName, options);
}

std::string GameEngine::createModel(const std::string& modelName, const Scene::_createModel_Options& options) {
    return scene_.createModel(modelName, options);
}

void GameEngine::destroyObject(const std::string& name) { scene_.destroyObject(name); }

GameObject& GameEngine::getObject(const std::string& name) const { return scene_.getObject(name); }

const std::unordered_map<std::string, GameObject>& GameEngine::getObjects() const { return scene_.getObjects(); }

nlohmann::json GameEngine::serializeScene() const { return SceneSerializer::serializeScene(scene_); }

void GameEngine::deserializeScene(const nlohmann::json& j) { SceneSerializer::deserializeScene(j, scene_); }

// --------------------------------------------------------
// Assets API
// --------------------------------------------------------

std::vector<std::string> GameEngine::getAvailableAssets(const std::string& extension) const {
    return assetManager_.getAvailableAssets(extension);
}
