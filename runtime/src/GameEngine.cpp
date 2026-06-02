#include "GameEngine.hpp"
#include "modules/asset_management/AssetCache.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/console/commands/EchoCommand.hpp"
#include "modules/console/commands/ListActorsCommand.hpp"
#include "modules/core/TimeManager.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/physics/PhysicsSystem.hpp"
#include "modules/rendering/GameRenderSystem.hpp"

GameEngine::GameEngine(GameWindow& window,
                       TimeManager& time,
                       AssetCache& loadedAssets,
                       InputSystem& inputSystem,
                       PhysicsSystem& physicsSystem,
                       World& entityManager,
                       GameRenderSystem& renderSystem,
                       RendererSettings& rendererSettings,
                       VulkanGameRenderer& renderer) :
    window_(window),
    time_(time),
    loadedAssets_(loadedAssets),
    inputSystem_(inputSystem),
    physicsSystem_(physicsSystem),
    entityManager_(entityManager),
    renderSystem_(renderSystem),
    rendererSettings_(rendererSettings),
    renderer_(renderer) {
    developerConsole_.registerCommand("list-actors",
                                      [this] { return std::make_unique<ListActorsCommand>(entityManager_); });
    developerConsole_.registerCommand("echo", [] { return std::make_unique<EchoCommand>(); });
}

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
        developerConsole_.tick();
        renderSystem_.update(entityManager_);
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
