#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>

struct InputComponent {
    std::unordered_map<int, bool> keys;
    std::unordered_map<int, bool> mouseButtons;
    double mouseX = 0.0;
    double mouseY = 0.0;
    double scrollX = 0.0;
    double scrollY = 0.0;

    InputComponent() {
        for (int key = GLFW_KEY_SPACE; key <= GLFW_KEY_LAST; ++key)
            keys[key] = false;
        for (int button = GLFW_MOUSE_BUTTON_1; button <= GLFW_MOUSE_BUTTON_LAST; ++button)
            mouseButtons[button] = false;
    }
};
