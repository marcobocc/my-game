#pragma once
#include <GLFW/glfw3.h>
#include <functional>
#include <stdexcept>
#include <utility>

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

        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        framebufferSize_ = {w, h};

        glfwSetWindowUserPointer(window_, this);
        glfwSetFramebufferSizeCallback(window_, [](GLFWwindow* win, int newWidth, int newHeight) {
            auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(win));
            if (!self) return;
            self->framebufferSize_ = {newWidth, newHeight};
            if (self->framebufferResizeHandler_) {
                self->framebufferResizeHandler_();
            }
        });
    }

    ~GameWindow() {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    GLFWwindow* get() const { return window_; }

    std::pair<int, int> getSize() const { return framebufferSize_; }
    void onFramebufferResize(std::function<void()> handler) {
        if (framebufferResizeHandler_) throw std::runtime_error("Framebuffer resize handler already set");
        framebufferResizeHandler_ = std::move(handler);
    }

    bool shouldClose() const { return glfwWindowShouldClose(window_); }
    void requestClose() const { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
    void pollEvents() const { glfwPollEvents(); }
    double getTime() const { return glfwGetTime(); }

private:
    GLFWwindow* window_;
    std::pair<int, int> framebufferSize_{};
    std::function<void()> framebufferResizeHandler_;
};
