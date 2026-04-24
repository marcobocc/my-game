#pragma once
#include <functional>
#include <memory>
#include "assets/AssetImporter.hpp"
#include "core/GameWindow.hpp"
#include "core/input/InputSystem.hpp"
#include "core/ui/UserInterface.hpp"
#include "rendering/RenderSystem.hpp"
#include "rendering/vulkan/VulkanWiringContainer.hpp"
#include "scene/Scene.hpp"

class GameEngine {
public:
    using GameLoopFunc = std::function<void(double deltaTime)>;

    GameEngine(const GameEngine&) = delete;
    GameEngine& operator=(const GameEngine&) = delete;
    GameEngine(GameEngine&&) = delete;
    GameEngine& operator=(GameEngine&&) = delete;

    explicit GameEngine(GameWindow& window, std::filesystem::path assetsPath);
    ~GameEngine();

    UserInterface& getUserInterface() const { return *userInterface_; }
    AssetManager& getAssetManager() const { return *assetManager_; }
    InputSystem& getInputSystem() const { return *inputSystem_; }
    Scene& getScene() const { return *scene_; }

    void run(const GameLoopFunc& gameLoopFunc);
    void requestClose() const;

private:
    void initialize(const std::filesystem::path& assetsPath);
    bool shouldClose() const;

    GameWindow& window_;
    std::unique_ptr<UserInterface> userInterface_;
    std::unique_ptr<VulkanWiringContainer> vulkanWiringContainer_;
    std::unique_ptr<RenderSystem> renderSystem_;
    std::unique_ptr<InputSystem> inputSystem_;
    std::unique_ptr<Scene> scene_;
    std::unique_ptr<AssetStorage> assetStorage_;
    std::unique_ptr<AssetImporter> assetImporter_;
    std::unique_ptr<AssetManager> assetManager_;

    GameLoopFunc updateFunction_;
    double lastFrameTime_;
};
