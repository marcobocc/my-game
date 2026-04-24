#pragma once
#include <unordered_map>
#include "core/GameWindow.hpp"

class InputSystem {
public:
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = delete;
    InputSystem& operator=(InputSystem&&) = delete;

    explicit InputSystem(GameWindow& window) : window_(window) {
        window_.onScroll([this](double yoffset) { scrollDelta_ += yoffset; });
    }

    void update() {
        scrollDeltaConsumed_ = scrollDelta_;
        scrollDelta_ = 0.0;
        prevKeys_ = keys_;
        updateKeys();
        updateMouseButtons();
    }

    bool isKeyDown(int key) const {
        auto it = keys_.find(key);
        return it != keys_.end() && it->second;
    }

    bool isKeyPressed(int key) const {
        auto cur = keys_.find(key);
        auto prev = prevKeys_.find(key);
        return (cur != keys_.end() && cur->second) && !(prev != prevKeys_.end() && prev->second);
    }

    bool isMouseButtonDown(int button) const {
        auto it = mouseButtons_.find(button);
        return it != mouseButtons_.end() && it->second;
    }

    std::pair<double, double> getMousePosition() const { return window_.getMousePosition(); }

    double getScrollDelta() const { return scrollDeltaConsumed_; }

private:
    void updateKeys() {
        for (int key = GameWindow::KEY_FIRST; key <= GameWindow::KEY_LAST; ++key)
            keys_[key] = window_.isKeyPressed(key);
    }

    void updateMouseButtons() {
        for (int button = GameWindow::MOUSE_BUTTON_FIRST; button <= GameWindow::MOUSE_BUTTON_LAST; ++button)
            mouseButtons_[button] = window_.isMouseButtonPressed(button);
    }

    GameWindow& window_;
    std::unordered_map<int, bool> keys_{};
    std::unordered_map<int, bool> prevKeys_{};
    std::unordered_map<int, bool> mouseButtons_{};
    double scrollDelta_ = 0.0;
    double scrollDeltaConsumed_ = 0.0;
};
