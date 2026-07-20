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
        prevMouseButtons_ = mouseButtons_;
        updateKeys();
        updateMouseButtons();
        auto [mx, my] = window_.getMousePosition();
        if (mouseDeltaInitialized_) {
            mouseDelta_ = {mx - prevMouseX_, my - prevMouseY_};
        } else {
            mouseDelta_ = {0.0, 0.0};
            mouseDeltaInitialized_ = true;
        }
        prevMouseX_ = mx;
        prevMouseY_ = my;
    }

    bool isKeyDown(int key) const {
        if (blocked_) return false;
        auto it = keys_.find(key);
        return it != keys_.end() && it->second;
    }

    bool isKeyPressed(int key) const {
        if (blocked_) return false;
        auto cur = keys_.find(key);
        auto prev = prevKeys_.find(key);
        return (cur != keys_.end() && cur->second) && !(prev != prevKeys_.end() && prev->second);
    }

    bool isMouseButtonDown(int button) const {
        if (blocked_ || mouseCapturedByUI_) return false;
        return isMouseButtonDownRaw(button);
    }

    // Raw variants ignore the UI mouse capture (but not the global block).
    // Used by the UI system itself, which must see input while capturing it.
    bool isMouseButtonDownRaw(int button) const {
        auto it = mouseButtons_.find(button);
        return it != mouseButtons_.end() && it->second;
    }

    bool wasMouseButtonDownRaw(int button) const {
        auto it = prevMouseButtons_.find(button);
        return it != prevMouseButtons_.end() && it->second;
    }

    bool isMouseButtonPressedRaw(int button) const {
        return isMouseButtonDownRaw(button) && !wasMouseButtonDownRaw(button);
    }

    std::pair<double, double> getMousePosition() const { return window_.getMousePosition(); }
    std::pair<double, double> getMouseDelta() const {
        return (blocked_ || mouseCapturedByUI_) ? std::pair{0.0, 0.0} : mouseDelta_;
    }

    double getScrollDelta() const { return (blocked_ || mouseCapturedByUI_) ? 0.0 : scrollDeltaConsumed_; }

    // Mouse-only consumption by the game UI: hides mouse buttons, delta and
    // scroll from scripts while the cursor interacts with UI. Keyboard unaffected.
    void setMouseCapturedByUI(bool captured) { mouseCapturedByUI_ = captured; }
    bool isMouseCapturedByUI() const { return mouseCapturedByUI_; }

    void lockMouse() const { window_.lockMouse(); }
    void unlockMouse() const { window_.unlockMouse(); }

    void setBlocked(bool blocked) {
        if (blocked == blocked_) return;
        blocked_ = blocked;
        // TODO: Add proper snapshotting for resume state before console was open
        if (blocked_) {
            mouseWasLocked_ = window_.isMouseLocked();
            window_.unlockMouse();
        } else if (mouseWasLocked_) {
            window_.lockMouse();
        }
    }
    bool isBlocked() const { return blocked_; }

    // Call when the simulation ends to prevent the play-mode mouse lock state
    // from being restored when setBlocked(false) fires on the next editor frame.
    void clearMouseLockState() { mouseWasLocked_ = false; }

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
    std::unordered_map<int, bool> prevMouseButtons_{};
    double scrollDelta_ = 0.0;
    double scrollDeltaConsumed_ = 0.0;
    double prevMouseX_ = 0.0;
    double prevMouseY_ = 0.0;
    std::pair<double, double> mouseDelta_{0.0, 0.0};
    bool mouseDeltaInitialized_ = false;
    bool blocked_ = false;
    bool mouseCapturedByUI_ = false;
    bool mouseWasLocked_ = false;
};
