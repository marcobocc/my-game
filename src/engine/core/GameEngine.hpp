#pragma once
#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include "core/input/InputSystem.hpp"
#include "rendering/RenderSystem.hpp"
#include "rendering/vulkan/VulkanGraphicsBackend.hpp"
#include "scene/Scene.hpp"

class GameEngine {
public:
    using GameLoopFunc = std::function<void(double deltaTime)>;

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine(GameEngine&&) = delete;
    GameEngine& operator=(GameEngine&&) = delete;

    explicit GameEngine(unsigned int windowWidth,
                        unsigned int windowHeight,
                        const char* windowTitle,
                        std::filesystem::path assetsPath);
    ~GameEngine();

    UserInterface& getUserInterface() const { return *userInterface_; }
    AssetManager& getAssetManager() const { return *assetManager_; }
    InputSystem& getInputSystem() const { return *inputSystem_; }
    Scene& getScene() const { return *scene_; }

    void run(const GameLoopFunc& gameLoopFunc);
    void requestClose() const;

private:
    void initialize(unsigned int windowWidth,
                    unsigned int windowHeight,
                    const char* windowTitle,
                    const std::filesystem::path& assetsPath);
    bool shouldClose() const;
    void handleResize();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* window_;
    std::unique_ptr<UserInterface> userInterface_;
    std::unique_ptr<VulkanGraphicsBackend> graphicsBackend_;
    std::unique_ptr<RenderSystem> renderSystem_;
    std::unique_ptr<InputSystem> inputSystem_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<AssetCache> assetCache_;
    std::unique_ptr<AssetManager> assetManager_;

    GameLoopFunc updateFunction_;
    double lastFrameTime_;
    bool framebufferResized_ = false;
};
