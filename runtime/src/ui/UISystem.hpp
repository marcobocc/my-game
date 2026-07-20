#pragma once
#include <memory>
#include <vector>
#include "Widgets.hpp"
#include "graphics/GameRenderData.hpp"

class InputSystem;
class GameWindow;

/*
    UISystem

    Screen-space game UI canvas. Owns the widget tree (windows and their
    child widgets), routes mouse input to widgets (consuming it before game
    scripts see it), and flattens the tree into UIDrawCalls each frame.
    Coordinates are logical scene-viewport pixels, origin top-left.
*/
class UISystem {
public:
    std::shared_ptr<UIWindow> createWindow();
    void destroyWindow(const std::shared_ptr<UIWindow>& window);
    void clear();

    // Hit-tests and dispatches mouse input. Must run after InputSystem::update()
    // and before game scripts tick.
    void update(InputSystem& input, const GameWindow& window);

    void buildDrawQueue(std::vector<UIDrawCall>& out, const GameWindow& window) const;

private:
    struct HitResult {
        std::shared_ptr<UIWindow> window;
        std::shared_ptr<UIButton> button;
    };

    HitResult hitTest(glm::vec2 mouse, glm::vec2 canvasSize) const;
    void bringToFront(UIWindow* window);
    static void emitWindow(const UIWindow& win, glm::vec2 canvasSize, std::vector<UIDrawCall>& out);
    static void emitQuad(std::vector<UIDrawCall>& out,
                         const std::string& texture,
                         glm::vec2 pos,
                         glm::vec2 size,
                         glm::vec4 color,
                         glm::vec2 uv0 = {0.0f, 0.0f},
                         glm::vec2 uv1 = {1.0f, 1.0f});
    static void emitNineSlice(std::vector<UIDrawCall>& out,
                              const std::string& texture,
                              glm::vec2 pos,
                              glm::vec2 size,
                              float border,
                              glm::vec4 color);

    std::vector<std::shared_ptr<UIWindow>> windows_;
    std::weak_ptr<UIButton> pressedButton_;
    std::weak_ptr<UIWindow> draggedWindow_;
    glm::vec2 lastMouse_{0.0f};
    int nextZOrder_ = 1;
};
