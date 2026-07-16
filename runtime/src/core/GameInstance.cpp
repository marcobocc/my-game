#include "GameInstance.hpp"
#include <tracy/Tracy.hpp>
#include "TimeManager.hpp"
#include "console/DeveloperConsole.hpp"
#include "console/commands/DebugDrawCommand.hpp"
#include "console/commands/EchoCommand.hpp"
#include "console/commands/ListActorsCommand.hpp"
#include "core/assets/AssetCache.hpp"
#include "graphics/GameRenderSystem.hpp"
#include "graphics/assets/Mesh.hpp"
#include "input/InputSystem.hpp"

GameInstance::GameInstance(GameWindow& window,
                           TimeManager& time,
                           AssetCache& loadedAssets,
                           AssetLoader& assetLoader,
                           InputSystem& inputSystem,
                           World world,
                           GameRenderSystem& renderSystem) :
    window_(window),
    time_(time),
    loadedAssets_(loadedAssets),
    assetLoader_(assetLoader),
    inputSystem_(inputSystem),
    world_(std::move(world)),
    physicsSystem_(world_),
    renderSystem_(renderSystem),
    animationSystem_(assetLoader) {

    developerConsole_.registerCommand("list-actors", [this] { return std::make_unique<ListActorsCommand>(world_); });
    developerConsole_.registerCommand("echo", [] { return std::make_unique<EchoCommand>(); });
    developerConsole_.registerCommand("debug", [this] { return std::make_unique<DebugDrawCommand>(debugDraw_); });

    luaScriptSystem_.init(world_, inputSystem_, debugDraw_);
    luaScriptSystem_.setMeshLookup([this](const std::string& name) -> const Mesh* {
        if (!loadedAssets_.contains<Mesh>(name)) return nullptr;
        return loadedAssets_.get<Mesh>(name);
    });
    assetLoader_.scanAndRegisterScripts(luaScriptSystem_);
    luaScriptSystem_.startInstances();

    physicsSystem_.onCollisionEnter([this](const PhysicsSystem::CollisionPair& pair) {
        luaScriptSystem_.callOnCollision(pair.first, pair.second);
        luaScriptSystem_.callOnCollision(pair.second, pair.first);
    });

    renderSystem_.setAnimationSystem(&animationSystem_);
}


// --------------------------------------------------------
// Game Loop
// --------------------------------------------------------

GameInstance::~GameInstance() { renderSystem_.setAnimationSystem(nullptr); }

void GameInstance::tick(float deltaTime) {
    debugDraw_.flush();
    debugDraw_.drawScene(world_);
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
bool GameInstance::isMouseButtonDown(int button) const { return inputSystem_.isMouseButtonDown(button); }
double GameInstance::getScrollDelta() const { return inputSystem_.getScrollDelta(); }

// --------------------------------------------------------
// Rendering API
// --------------------------------------------------------

void GameInstance::setActiveCamera(const Camera& camera, const Transform& transform) {
    renderSystem_.setActiveCamera(camera, transform);
}
