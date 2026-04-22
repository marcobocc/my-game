#pragma once
#include <GLFW/glfw3.h>
#include <functional>
#include <stdexcept>
#include <utility>

struct SceneViewport {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

class GameWindow {
public:
    GameWindow(const GameWindow&) = delete;
    GameWindow& operator=(const GameWindow&) = delete;
    GameWindow(GameWindow&&) = delete;
    GameWindow& operator=(GameWindow&&) = delete;

    GameWindow(size_t width, size_t height, const char* title) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize window");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

        if (!window_) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        int w = 0, h = 0;
        glfwGetFramebufferSize(window_, &w, &h);
        framebufferSize_ = {w, h};
        sceneViewport_ = {0, 0, w, h};

        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* win, int newWidth, int newHeight) {
            auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(win));
            if (!self) return;
            int oldWidth = self->framebufferSize_.first;
            int oldHeight = self->framebufferSize_.second;
            self->framebufferSize_ = {newWidth, newHeight};
            for (const auto& handler: self->framebufferResizeHandlers_)
                handler(newWidth, newHeight, oldWidth, oldHeight);
        });
    }

    ~GameWindow() {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    GLFWwindow* get() const { return window_; }

    std::pair<int, int> getSize() const { return framebufferSize_; }
    void onFramebufferResize(std::function<void(int, int, int, int)> handler) {
        framebufferResizeHandlers_.push_back(std::move(handler));
    }

    SceneViewport getSceneViewport() const { return sceneViewport_; }
    void setSceneViewport(SceneViewport viewport) { sceneViewport_ = viewport; }

    bool shouldClose() const { return glfwWindowShouldClose(window_); }
    void requestClose() const { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
    void pollEvents() const { glfwPollEvents(); }
    double getTime() const { return glfwGetTime(); }

private:
    GLFWwindow* window_;
    std::pair<int, int> framebufferSize_{};
    SceneViewport sceneViewport_{};
    std::vector<std::function<void(int, int, int, int)>> framebufferResizeHandlers_;
};
