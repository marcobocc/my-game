#include "GameEngine.hpp"


GameEngine::GameEngine(GameWindow& window, std::filesystem::path assetsPath) : window_(window), lastFrameTime_(0.0) {
    initialize(assetsPath);
}

GameEngine::~GameEngine() {
    inputSystem_.reset();
    renderSystem_.reset();
    vulkanWiringContainer_.reset();
    scene_.reset();
}

void GameEngine::initialize(const std::filesystem::path& assetsPath) {
    userInterface_ = std::make_unique<UserInterface>();
    assetStorage_ = std::make_unique<AssetStorage>();
    assetImporter_ = std::make_unique<AssetImporter>(assetsPath, *assetStorage_);
    assetManager_ = std::make_unique<AssetManager>(*assetImporter_, *assetStorage_);
    scene_ = std::make_unique<Scene>();
    vulkanWiringContainer_ = std::make_unique<VulkanWiringContainer>(window_, *assetManager_, *userInterface_);
    renderSystem_ = std::make_unique<RenderSystem>(vulkanWiringContainer_->graphicsBackend());
    inputSystem_ = std::make_unique<InputSystem>(window_.get());
    lastFrameTime_ = window_.getTime();
}

void GameEngine::run(const GameLoopFunc& gameLoopFunc) {
    while (!shouldClose()) {
        double currentTime = window_.getTime();
        double deltaTime = currentTime - lastFrameTime_;
        lastFrameTime_ = currentTime;

        window_.pollEvents();
        inputSystem_->update();

        gameLoopFunc(deltaTime);
        renderSystem_->update(*scene_);
    }
}

bool GameEngine::shouldClose() const { return window_.shouldClose(); }

void GameEngine::requestClose() const { window_.requestClose(); }
