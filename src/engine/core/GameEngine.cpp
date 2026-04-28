#include "GameEngine.hpp"
#include <glm/vec3.hpp>


GameEngine::GameEngine(GameWindow& window, std::filesystem::path assetsPath) : window_(window) {
    initialize(assetsPath);
}

GameEngine::~GameEngine() {
    inputSystem_.reset();
    pickingSystem_.reset();
    physicsSystem_.reset();
    renderSystem_.reset();
    vulkanWiringContainer_.reset();
    scene_.reset();
}

void GameEngine::initialize(const std::filesystem::path& assetsPath) {
    userInterface_ = std::make_unique<UserInterface>();
    assetStorage_ = std::make_unique<AssetStorage>();
    assetImporter_ = std::make_unique<AssetImporter>(assetsPath, *assetStorage_);
    assetManager_ = std::make_unique<AssetManager>(*assetImporter_, *assetStorage_);
    scene_ = std::make_unique<Scene>(*assetManager_);
    physicsSystem_ = std::make_unique<PhysicsSystem>(*scene_);
    vulkanWiringContainer_ =
            std::make_unique<VulkanWiringContainer>(window_, *assetManager_, *userInterface_, rendererSettings_);
    pickingSystem_ = std::make_unique<PickingSystem>(vulkanWiringContainer_->graphicsBackend().pickingBackend());
    renderSystem_ = std::make_unique<RenderSystem>(vulkanWiringContainer_->graphicsBackend());
    inputSystem_ = std::make_unique<InputSystem>(window_);
    time_ = std::make_unique<Time>([this] { return static_cast<float>(window_.getTime()); });
}

void GameEngine::run(const GameLoopFunc& gameLoopFunc) {
    while (!shouldClose()) {
        time_->beginFrame();
        float deltaTime = time_->getGameDeltaTime();

        window_.pollEvents();
        inputSystem_->update();
        physicsSystem_->update();

        gameLoopFunc(deltaTime);
        renderSystem_->update(*scene_);
        time_->endFrame();
    }
}

bool GameEngine::shouldClose() const { return window_.shouldClose(); }

void GameEngine::requestClose() const { window_.requestClose(); }

void GameEngine::outline(const Renderer& renderer, const Transform& transform, std::string objectId) const {
    vulkanWiringContainer_->graphicsBackend().outline(renderer, transform, std::move(objectId));
}

void GameEngine::submitGizmoLine(glm::vec3 from, glm::vec3 to, glm::vec3 color) const {
    vulkanWiringContainer_->graphicsBackend().submitGizmoLine(from, to, color);
}
