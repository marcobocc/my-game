#include "GameEngine.hpp"
#include <stdexcept>


GameEngine::GameEngine(unsigned int windowWidth, unsigned int windowHeight, const char* windowTitle) :
    window_(nullptr),
    lastFrameTime_(0.0) {
    initialize(windowWidth, windowHeight, windowTitle);
}

GameEngine::~GameEngine() {
    inputSystem_.reset();
    renderSystem_.reset();
    graphicsBackend_.reset();
    ecs_.reset();

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void GameEngine::initialize(unsigned int windowWidth, unsigned int windowHeight, const char* windowTitle) {
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window_ = glfwCreateWindow(windowWidth, windowHeight, windowTitle, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    ecs_ = std::make_unique<GameEntitiesManager>();
    graphicsBackend_ = std::make_unique<VulkanGraphicsBackend>(window_);
    assetManager_ = std::make_unique<AssetManager>();
    renderSystem_ = std::make_unique<RenderSystem>(*ecs_, *graphicsBackend_);
    playerInput_ = std::make_unique<InputComponent>();
    inputSystem_ = std::make_unique<InputSystem>(window_, *playerInput_);
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
        renderSystem_->update();
    }
}

bool GameEngine::shouldClose() const { return glfwWindowShouldClose(window_); }

void GameEngine::requestClose() const { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
