#include "GameEngine.hpp"
#include <stdexcept>


GameEngine::GameEngine(unsigned int windowWidth,
                       unsigned int windowHeight,
                       const char* windowTitle,
                       std::filesystem::path assetsPath) :
    window_(nullptr),
    lastFrameTime_(0.0) {
    initialize(windowWidth, windowHeight, windowTitle, assetsPath);
}

GameEngine::~GameEngine() {
    inputSystem_.reset();
    renderSystem_.reset();
    graphicsBackend_.reset();
    scene_.reset();

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void GameEngine::initialize(unsigned int windowWidth,
                            unsigned int windowHeight,
                            const char* windowTitle,
                            const std::filesystem::path& assetsPath) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    userInterface_ = std::make_unique<UserInterface>();
    assetCache_ = std::make_unique<AssetCache>();
    assetManager_ = std::make_unique<AssetManager>(*assetCache_, assetsPath);
    scene_ = std::make_unique<Scene>();
    graphicsBackend_ = std::make_unique<VulkanGraphicsBackend>(*assetManager_, window_, userInterface_.get());
    renderSystem_ = std::make_unique<RenderSystem>(*graphicsBackend_);
    inputSystem_ = std::make_unique<InputSystem>(window_);
    lastFrameTime_ = glfwGetTime();
}


void GameEngine::run(const GameLoopFunc& gameLoopFunc) {
    while (!shouldClose()) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastFrameTime_;
        lastFrameTime_ = currentTime;

        glfwPollEvents();
        inputSystem_->update();

        gameLoopFunc(deltaTime);
        renderSystem_->update(*scene_);
    }
}

bool GameEngine::shouldClose() const { return glfwWindowShouldClose(window_); }

void GameEngine::requestClose() const { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
