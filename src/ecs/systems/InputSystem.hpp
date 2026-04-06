#pragma once
#include <GLFW/glfw3.h>
#include "ecs/components/InputComponent.hpp"

class InputSystem {
public:
    InputSystem(GLFWwindow* window, InputComponent& input_component) :
        window_(window),
        input_component_(input_component) {}

    void update() const {
        updateKeys();
        updateMouseButtons();
        updateCursor();
    }

private:
    void updateKeys() const {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
            input_component_.keys[key] = glfwGetKey(window_, key) == GLFW_PRESS;
    }

    void updateMouseButtons() const {
        for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
            input_component_.mouseButtons[button] = glfwGetMouseButton(window_, button) == GLFW_PRESS;
    }

    void updateCursor() const { glfwGetCursorPos(window_, &input_component_.mouseX, &input_component_.mouseY); }

    GLFWwindow* window_;
    InputComponent& input_component_;
};
