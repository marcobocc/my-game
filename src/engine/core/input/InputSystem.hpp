#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>

class InputSystem {
public:
    explicit InputSystem(GLFWwindow* window) : window_(window) {}

    void update() {
        updateKeys();
        updateMouseButtons();
        updateCursor();
    }

    bool isKeyPressed(int key) const {
        auto it = keys_.find(key);
        return it != keys_.end() && it->second;
    }

    bool isMouseButtonPressed(int button) const {
        auto it = mouseButtons_.find(button);
        return it != mouseButtons_.end() && it->second;
    }

    std::pair<double, double> getMousePosition() const {
        return std::make_pair(mouseX_, mouseY_);
    }

private:
    void updateKeys() {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
            keys_[key] = glfwGetKey(window_, key) == GLFW_PRESS;
    }

    void updateMouseButtons() {
        for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
            mouseButtons_[button] = glfwGetMouseButton(window_, button) == GLFW_PRESS;
    }

    void updateCursor() { glfwGetCursorPos(window_, &mouseX_, &mouseY_); }

    GLFWwindow* window_;
    std::unordered_map<int, bool> keys_{};
    std::unordered_map<int, bool> mouseButtons_{};
    double mouseX_ = 0.0;
    double mouseY_ = 0.0;
};
