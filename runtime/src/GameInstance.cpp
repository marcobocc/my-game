#include "GameInstance.hpp"
#include <tracy/Tracy.hpp>
#include "modules/asset_management/AssetCache.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/console/commands/EchoCommand.hpp"
#include "modules/console/commands/ListActorsCommand.hpp"
#include "modules/core/TimeManager.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/rendering/GameRenderSystem.hpp"

GameInstance::GameInstance(GameWindow& window,
                           TimeManager& time,
                           AssetCache& loadedAssets,
                           AssetLoader& assetLoader,
                           InputSystem& inputSystem,
                           World world,
                           GameRenderSystem& renderSystem,
                           RendererSettings& rendererSettings,
                           IGameRenderer& renderer) :
    window_(window),
    time_(time),
    loadedAssets_(loadedAssets),
    inputSystem_(inputSystem),
    world_(std::move(world)),
    physicsSystem_(world_),
    renderSystem_(renderSystem),
    rendererSettings_(rendererSettings),
    renderer_(renderer),
    animationSystem_(assetLoader) {
    developerConsole_.registerCommand("list-actors", [this] { return std::make_unique<ListActorsCommand>(world_); });
    developerConsole_.registerCommand("echo", [] { return std::make_unique<EchoCommand>(); });

    luaScriptSystem_.init(world_, inputSystem_, std::filesystem::path(ENGINE_DATA_DIR) / "scripts");
    renderSystem_.setAnimationSystem(&animationSystem_);
}


// --------------------------------------------------------
// Game Loop
// --------------------------------------------------------

GameInstance::~GameInstance() { renderSystem_.setAnimationSystem(nullptr); }

void GameInstance::tick(float deltaTime) {
    physicsSystem_.update();
    animationSystem_.update(deltaTime, world_);
    luaScriptSystem_.update(deltaTime);
    developerConsole_.tick();
}

void GameInstance::run(const std::function<void(double deltaTime)>& gameLoopFunc) {
    while (!shouldClose()) {
        ZoneScoped;
        time_.beginFrame();
        float deltaTime = time_.getGameDeltaTime();

        window_.pollEvents();
        {
            ZoneScopedN("Input");
            inputSystem_.update();
        }
        {
            ZoneScopedN("Tick");
            tick(deltaTime);
            gameLoopFunc(deltaTime);
        }
        {
            ZoneScopedN("Render");
            renderSystem_.setDeltaTime(deltaTime);
            renderSystem_.update(world_);
        }
        time_.endFrame();
        FrameMark;
    }
}

bool GameInstance::shouldClose() const { return window_.shouldClose(); }
void GameInstance::requestClose() const { window_.requestClose(); }

// --------------------------------------------------------
// Input API
// --------------------------------------------------------

std::pair<double, double> GameInstance::getMousePosition() const { return inputSystem_.getMousePosition(); }
std::pair<double, double> GameInstance::getMouseDelta() const { return inputSystem_.getMouseDelta(); }
void GameInstance::lockMouse() const { inputSystem_.lockMouse(); }
void GameInstance::unlockMouse() const { inputSystem_.unlockMouse(); }
bool GameInstance::isKeyDown(int key) const { return inputSystem_.isKeyDown(key); }
bool GameInstance::isKeyPressed(int key) const { return inputSystem_.isKeyPressed(key); }
bool GameInstance::isMouseButtonDown(int btn) const { return inputSystem_.isMouseButtonDown(btn); }
double GameInstance::getScrollDelta() const { return inputSystem_.getScrollDelta(); }

// --------------------------------------------------------
// Rendering API
// --------------------------------------------------------

void GameInstance::setActiveCamera(const Camera& camera, const Transform& transform) {
    renderSystem_.setActiveCamera(camera, transform);
}
