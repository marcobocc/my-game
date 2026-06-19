#pragma once
#include <filesystem>
#include <memory>
#include <stdexcept>
#include "../../../runtime/src/GameInstance.hpp"
#include "../../../runtime/src/modules/input/InputSystem.hpp"
#include "../../../runtime/src/modules/physics/PhysicsSystem.hpp"
#include "../../../runtime/src/modules/rendering/GameRenderSystem.hpp"
#include "../../../runtime/src/modules/scene/World.hpp"

class GameWindow;
class TimeManager;
class AssetCache;
#include "modules/rendering/IGameRenderer.hpp"
struct RendererSettings;

class SimulationController {
public:
    enum class State { Stopped, Running, Paused };

    SimulationController(GameWindow& window,
                         TimeManager& time,
                         AssetCache& loadedAssets,
                         InputSystem& inputSystem,
                         GameRenderSystem& renderSystem,
                         RendererSettings& rendererSettings,
                         IGameRenderer& renderer) :
        window_(window),
        time_(time),
        loadedAssets_(loadedAssets),
        inputSystem_(inputSystem),
        renderSystem_(renderSystem),
        rendererSettings_(rendererSettings),
        renderer_(renderer) {}

    void setEditorWorld(World* editorWorld) { editorWorld_ = editorWorld; }
    void setProjectRoot(const std::filesystem::path& projectRoot) { projectRoot_ = projectRoot; }

    void requestPlay() {
        if (state_ != State::Stopped || !editorWorld_) return;
        start(editorWorld_->snapshot());
    }

    void start(World snapshot) {
        gameInstance_ = std::make_unique<GameInstance>(window_,
                                                       time_,
                                                       loadedAssets_,
                                                       inputSystem_,
                                                       std::move(snapshot),
                                                       renderSystem_,
                                                       rendererSettings_,
                                                       renderer_);
        gameInstance_->physicsSystem().resume();
        state_ = State::Running;
    }

    void pause() {
        if (state_ == State::Running && gameInstance_) {
            gameInstance_->physicsSystem().pause();
            state_ = State::Paused;
        }
    }

    void resume() {
        if (state_ == State::Paused && gameInstance_) {
            gameInstance_->physicsSystem().resume();
            state_ = State::Running;
        }
    }

    void stop() { pendingStop_ = true; }

    void flushPendingStop() {
        if (!pendingStop_) return;
        pendingStop_ = false;
        gameInstance_.reset();
        state_ = State::Stopped;
        inputSystem_.clearMouseLockState();
        window_.unlockMouse();
    }

    void tick(double deltaTime) {
        if (state_ != State::Running || !gameInstance_) return;
        gameInstance_->tick(static_cast<float>(deltaTime));
    }

    bool isActive() const { return state_ != State::Stopped && !pendingStop_; }
    bool isRunning() const { return state_ == State::Running; }
    bool isPaused() const { return state_ == State::Paused; }

    GameInstance* gameInstance() { return gameInstance_.get(); }
    World& world() {
        if (!gameInstance_) throw std::runtime_error("No active simulation");
        return gameInstance_->entities();
    }

private:
    GameWindow& window_;
    TimeManager& time_;
    AssetCache& loadedAssets_;
    InputSystem& inputSystem_;
    GameRenderSystem& renderSystem_;
    RendererSettings& rendererSettings_;
    IGameRenderer& renderer_;

    World* editorWorld_ = nullptr;
    std::filesystem::path projectRoot_;
    std::unique_ptr<GameInstance> gameInstance_;
    State state_ = State::Stopped;
    bool pendingStop_ = false;
};
