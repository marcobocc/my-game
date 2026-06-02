#include "GameInstance.hpp"
#include "modules/asset_management/AssetCache.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/console/commands/EchoCommand.hpp"
#include "modules/console/commands/ListActorsCommand.hpp"
#include "modules/core/TimeManager.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/rendering/GameRenderSystem.hpp"
#include "modules/scene/components/BehaviourScript.hpp"
#include "modules/scripting/ScriptInstanceContext.hpp"

GameInstance::GameInstance(GameWindow& window,
                           TimeManager& time,
                           AssetCache& loadedAssets,
                           InputSystem& inputSystem,
                           World world,
                           GameRenderSystem& renderSystem,
                           RendererSettings& rendererSettings,
                           VulkanGameRenderer& renderer,
                           std::filesystem::path scriptsDir) :
    window_(window),
    time_(time),
    loadedAssets_(loadedAssets),
    inputSystem_(inputSystem),
    world_(std::move(world)),
    physicsSystem_(world_),
    renderSystem_(renderSystem),
    rendererSettings_(rendererSettings),
    renderer_(renderer),
    scriptManager_(ScriptInstanceContext{&world_}) {
    developerConsole_.registerCommand("list-actors", [this] { return std::make_unique<ListActorsCommand>(world_); });
    developerConsole_.registerCommand("echo", [] { return std::make_unique<EchoCommand>(); });

    if (!scriptsDir.empty()) {
        for (auto& [handle, script]: world_.query<BehaviourScript>()) {
            if (script->scriptName.empty()) continue;
            std::string stem = std::filesystem::path(script->scriptName).stem().string();
            auto dylibPath = scriptsDir / (stem + ".dylib");
            scriptManager_.loadScript(stem, dylibPath);
            scriptManager_.spawnInstance(stem, handle);
        }
    }
}

// --------------------------------------------------------
// Game Loop
// --------------------------------------------------------

void GameInstance::tick(float deltaTime) {
    physicsSystem_.update();
    scriptManager_.tickAll(deltaTime);
    developerConsole_.tick();
}

void GameInstance::run(const std::function<void(double deltaTime)>& gameLoopFunc) {
    while (!shouldClose()) {
        time_.beginFrame();
        float deltaTime = time_.getGameDeltaTime();

        window_.pollEvents();
        inputSystem_.update();
        tick(deltaTime);
        gameLoopFunc(deltaTime);
        renderSystem_.update(world_);
        time_.endFrame();
    }
}

bool GameInstance::shouldClose() const { return window_.shouldClose(); }

void GameInstance::requestClose() const { window_.requestClose(); }

// --------------------------------------------------------
// Input API
// --------------------------------------------------------

std::pair<double, double> GameInstance::getMousePosition() const { return inputSystem_.getMousePosition(); }

bool GameInstance::isKeyDown(int key) const { return inputSystem_.isKeyDown(key); }

bool GameInstance::isKeyPressed(int key) const { return inputSystem_.isKeyPressed(key); }

bool GameInstance::isMouseButtonDown(int button) const { return inputSystem_.isMouseButtonDown(button); }

double GameInstance::getScrollDelta() const { return inputSystem_.getScrollDelta(); }

// --------------------------------------------------------
// Rendering API
// --------------------------------------------------------

void GameInstance::setActiveCamera(const Camera& camera, const Transform& transform) {
    renderSystem_.setActiveCamera(camera, transform);
}
