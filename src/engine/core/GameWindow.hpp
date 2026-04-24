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
        if (!glfwInit()) throw std::runtime_error("Failed to initialize window");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title, nullptr, nullptr);
        if (!window_) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }
        int lw = 0, lh = 0;
        glfwGetWindowSize(window_, &lw, &lh);
        logicalSize_ = {lw, lh};
        sceneViewport_ = {0, 0, lw, lh};
        glfwSetWindowUserPointer(window_, this);
        glfwSetWindowSizeCallback(window_, [](GLFWwindow* win, int newWidth, int newHeight) {
            auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(win));
            if (!self) return;
            int oldWidth = self->logicalSize_.first;
            int oldHeight = self->logicalSize_.second;
            self->logicalSize_ = {newWidth, newHeight};
            for (const auto& handler: self->windowResizeHandlers_)
                handler(newWidth, newHeight, oldWidth, oldHeight);
        });
        glfwSetScrollCallback(window_, [](GLFWwindow* win, double /*xoffset*/, double yoffset) {
            auto* self = static_cast<GameWindow*>(glfwGetWindowUserPointer(win));
            if (!self) return;
            for (const auto& handler: self->scrollHandlers_)
                handler(yoffset);
        });
    }

    ~GameWindow() {
        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    GLFWwindow* get() const { return window_; }

    std::pair<int, int> getLogicalSize() const { return logicalSize_; }

    // Returns the scale factor from logical to framebuffer (physical pixel) coordinates.
    // On high-DPI displays this is typically {2, 2}; on standard displays {1, 1}.
    std::pair<float, float> getContentScale() const {
        float sx = 1.0f, sy = 1.0f;
        glfwGetWindowContentScale(window_, &sx, &sy);
        return {sx, sy};
    }

    void onWindowResize(std::function<void(int, int, int, int)> handler) {
        windowResizeHandlers_.push_back(std::move(handler));
    }

    void onScroll(std::function<void(double)> handler) { scrollHandlers_.push_back(std::move(handler)); }

    static constexpr int KEY_FIRST = GLFW_KEY_SPACE;
    static constexpr int KEY_LAST = GLFW_KEY_LAST;
    static constexpr int MOUSE_BUTTON_FIRST = GLFW_MOUSE_BUTTON_1;
    static constexpr int MOUSE_BUTTON_LAST = GLFW_MOUSE_BUTTON_LAST;

    bool isKeyPressed(int key) const { return glfwGetKey(window_, key) == GLFW_PRESS; }
    bool isMouseButtonPressed(int button) const { return glfwGetMouseButton(window_, button) == GLFW_PRESS; }
    std::pair<double, double> getMousePosition() const {
        double x = 0.0, y = 0.0;
        glfwGetCursorPos(window_, &x, &y);
        return {x, y};
    }

    SceneViewport getSceneViewport() const { return sceneViewport_; }
    void setSceneViewport(SceneViewport viewport) { sceneViewport_ = viewport; }

    bool shouldClose() const { return glfwWindowShouldClose(window_); }
    void requestClose() const { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
    void pollEvents() const { glfwPollEvents(); }
    double getTime() const { return glfwGetTime(); }

private:
    GLFWwindow* window_;
    std::pair<int, int> logicalSize_{};
    SceneViewport sceneViewport_{};
    std::vector<std::function<void(int, int, int, int)>> windowResizeHandlers_;
    std::vector<std::function<void(double)>> scrollHandlers_;
};
