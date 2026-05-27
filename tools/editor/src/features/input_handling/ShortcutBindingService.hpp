#pragma once
#include <GLFW/glfw3.h>
#include <optional>
#include <unordered_map>
#include <vector>
#include "../../services/ActionDispatcher.hpp"
#include "Shortcut.hpp"

class ShortcutBindingService {
public:
    struct ShortcutBinding {
        ActionID action = ActionID::UNKNOWN;
        Shortcut shortcut;
    };

    ShortcutBindingService() { setDefaultBindings(); }

    std::optional<ActionID> getAction(const Shortcut& shortcut) const {
        auto it = shortcutToActionMap_.find(shortcut);
        if (it != shortcutToActionMap_.end()) return it->second;
        return std::nullopt;
    }

    std::string getShortcut(ActionID id) const {
        for (const auto& binding: shortcutToActionMap_) {
            if (binding.second == id) return binding.first.toString();
        }
        return "";
    }

private:
    void setDefaultBindings() {
        static const std::vector<ShortcutBinding> DEFAULT_BINDINGS = {
                // CTRL + SHIFT + ALT + KEY
                {ActionID::UNDO, {true, false, false, GLFW_KEY_Z}},
                {ActionID::REDO, {true, true, false, GLFW_KEY_Z}},
                {ActionID::COPY, {true, false, false, GLFW_KEY_C}},
                {ActionID::CUT, {true, false, false, GLFW_KEY_X}},
                {ActionID::PASTE, {true, false, false, GLFW_KEY_V}},
                {ActionID::DUPLICATE, {true, false, false, GLFW_KEY_D}},
                {ActionID::DELETE, {true, true, false, GLFW_KEY_X}},
                {ActionID::TOGGLE_GRID, {false, false, false, GLFW_KEY_G}},
                {ActionID::TOGGLE_LIGHTING, {false, false, false, GLFW_KEY_L}},
                {ActionID::TOGGLE_BVH, {false, false, false, GLFW_KEY_B}},
                {ActionID::GIZMO_TRANSLATE, {false, false, false, GLFW_KEY_T}},
                {ActionID::GIZMO_ROTATE, {false, false, false, GLFW_KEY_R}},
                {ActionID::GIZMO_SCALE, {false, false, false, GLFW_KEY_Y}},
                {ActionID::CAMERA_RESET, {false, false, false, GLFW_KEY_F}},
                {ActionID::INCREASE_SCALE, {false, false, false, GLFW_KEY_KP_ADD}},
                {ActionID::DECREASE_SCALE, {false, false, false, GLFW_KEY_KP_SUBTRACT}},
                {ActionID::TOGGLE_SNAPPING, {false, false, false, GLFW_KEY_U}},
                {ActionID::SAVE, {true, false, false, GLFW_KEY_S}}};

        for (const auto& binding: DEFAULT_BINDINGS) {
            shortcutToActionMap_[binding.shortcut] = binding.action;
        }
    }

    std::unordered_map<Shortcut, ActionID> shortcutToActionMap_;
};
