#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include "core/GameEntitiesManager.hpp"
#include "core/components/InputComponent.hpp"
#include "core/systems/InputSystem.hpp"
#include "core/systems/RenderSystem.hpp"
#include "vulkan/VulkanGraphicsBackend.hpp"

class GameEngine {
public:
    using GameLoopFunc = std::function<void(double deltaTime)>;

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine(GameEngine&&) = delete;
    GameEngine& operator=(GameEngine&&) = delete;

    explicit GameEngine(unsigned int windowWidth, unsigned int windowHeight, const char* windowTitle);
    ~GameEngine();

    GameEntitiesManager& getECS() const { return *ecs_; }
    const InputComponent& getPlayerInput() const { return *playerInput_; }

    void run(const GameLoopFunc& gameLoopFunc);
    void requestClose() const;

private:
    void initialize(unsigned int windowWidth, unsigned int windowHeight, const char* windowTitle);
    bool shouldClose() const;

    GLFWwindow* window_;
    std::unique_ptr<GameEntitiesManager> ecs_;
    std::unique_ptr<VulkanGraphicsBackend> graphicsBackend_;
    std::unique_ptr<RenderSystem> renderSystem_;
    std::unique_ptr<InputSystem> inputSystem_;
    std::unique_ptr<InputComponent> playerInput_;

    GameLoopFunc updateFunction_;
    double lastFrameTime_;
};
