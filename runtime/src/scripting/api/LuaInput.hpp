#pragma once
#include <log4cxx/logger.h>
#include <string>
#include <unordered_map>
#include "input/InputSystem.hpp"

class LuaInput {
public:
    explicit LuaInput(InputSystem& input) : input_(input) {}

    bool isKeyDown(const std::string& name) const {
        int key = resolve(name);
        return key >= 0 && input_.isKeyDown(key);
    }

    bool isKeyPressed(const std::string& name) const {
        int key = resolve(name);
        return key >= 0 && input_.isKeyPressed(key);
    }

    bool isMouseButtonDown(const std::string& name) const {
        static const std::unordered_map<std::string, int> kMouseMap = {
                {"LEFT", 0},
                {"RIGHT", 1},
                {"MIDDLE", 2},
        };
        auto it = kMouseMap.find(name);
        if (it == kMouseMap.end()) {
            LOG4CXX_WARN(log4cxx::Logger::getLogger("LuaInput"), "Unknown mouse button: " << name);
            return false;
        }
        return input_.isMouseButtonDown(it->second);
    }

    std::pair<double, double> getMouseDelta() const { return input_.getMouseDelta(); }
    std::pair<double, double> getMousePosition() const { return input_.getMousePosition(); }
    double getScrollDelta() const { return input_.getScrollDelta(); }

    void lockMouse() const { input_.lockMouse(); }
    void unlockMouse() const { input_.unlockMouse(); }

private:
    InputSystem& input_;

    int resolve(const std::string& name) const {
        // GLFW key codes — platform detail hidden from scripts
        static const std::unordered_map<std::string, int> kKeyMap = {
                {"A", 65},      {"B", 66},      {"C", 67},       {"D", 68},       {"E", 69},          {"F", 70},
                {"G", 71},      {"H", 72},      {"I", 73},       {"J", 74},       {"K", 75},          {"L", 76},
                {"M", 77},      {"N", 78},      {"O", 79},       {"P", 80},       {"Q", 81},          {"R", 82},
                {"S", 83},      {"T", 84},      {"U", 85},       {"V", 86},       {"W", 87},          {"X", 88},
                {"Y", 89},      {"Z", 90},      {"0", 48},       {"1", 49},       {"2", 50},          {"3", 51},
                {"4", 52},      {"5", 53},      {"6", 54},       {"7", 55},       {"8", 56},          {"9", 57},
                {"SPACE", 32},  {"ENTER", 257}, {"ESCAPE", 256}, {"TAB", 258},    {"BACKSPACE", 259}, {"LEFT", 263},
                {"RIGHT", 262}, {"UP", 265},    {"DOWN", 264},   {"LSHIFT", 340}, {"RSHIFT", 344},    {"LCTRL", 341},
                {"RCTRL", 345}, {"LALT", 342},  {"RALT", 346},   {"F1", 290},     {"F2", 291},        {"F3", 292},
                {"F4", 293},    {"F5", 294},    {"F6", 295},     {"F7", 296},     {"F8", 297},        {"F9", 298},
                {"F10", 299},   {"F11", 300},   {"F12", 301},
        };
        auto it = kKeyMap.find(name);
        if (it == kKeyMap.end()) {
            LOG4CXX_WARN(log4cxx::Logger::getLogger("LuaInput"), "Unknown key name: " << name);
            return -1;
        }
        return it->second;
    }
};
