#pragma once
#include <GLFW/glfw3.h>
#include "../../services/utils/StringUtils.hpp"

struct Shortcut {
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    int key = GLFW_KEY_UNKNOWN;

    std::string toString() const {
        std::string out;
#ifdef __APPLE__
        if (ctrl) out += "Cmd + ";
#else
        if (ctrl) out += "Ctrl + ";
#endif
        if (shift) out += "Shift + ";
        if (alt) out += "Alt + ";
        if (const char* name = glfwGetKeyName(key, 0)) out += toTitleCase(name);
        return out;
    }

    bool operator==(const Shortcut& other) const noexcept {
        return ctrl == other.ctrl && shift == other.shift && alt == other.alt && key == other.key;
    }
};

template<>
struct std::hash<Shortcut> {
    size_t operator()(const Shortcut& s) const noexcept {
        size_t h = 0;
        auto combine = [&h](size_t v) { h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2); };
        combine(std::hash<bool>{}(s.ctrl));
        combine(std::hash<bool>{}(s.shift));
        combine(std::hash<bool>{}(s.alt));
        combine(std::hash<int>{}(s.key));
        return h;
    }
};
