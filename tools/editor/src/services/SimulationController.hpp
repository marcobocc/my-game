#pragma once
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
class VulkanGameRenderer;
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
                         VulkanGameRenderer& renderer) :
        window_(window),
        time_(time),
        loadedAssets_(loadedAssets),
        inputSystem_(inputSystem),
        renderSystem_(renderSystem),
        rendererSettings_(rendererSettings),
        renderer_(renderer) {}

    void setEditorWorld(World* editorWorld) { editorWorld_ = editorWorld; }

    // Called by the Play button — snapshots the editor world and starts simulation.
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

    // Safe to call from inside a draw callback — actual teardown is deferred to flushPendingStop().
    void stop() { pendingStop_ = true; }

    // Called at the top of the main loop, before any rendering.
    void flushPendingStop() {
        if (!pendingStop_) return;
        pendingStop_ = false;
        gameInstance_.reset();
        state_ = State::Stopped;
    }

    void tick(double deltaTime) {
        if (state_ != State::Running || !gameInstance_) return;

        inputSystem_.update();
        gameInstance_->physicsSystem().update();
        gameInstance_->developerConsole().tick();
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
    VulkanGameRenderer& renderer_;

    World* editorWorld_ = nullptr;
    std::unique_ptr<GameInstance> gameInstance_;
    State state_ = State::Stopped;
    bool pendingStop_ = false;
};
