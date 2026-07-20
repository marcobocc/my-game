#include "UISystem.hpp"
#include <algorithm>
#include "core/GameWindow.hpp"
#include "input/InputSystem.hpp"

namespace {
    glm::vec2 canvasSizeOf(const GameWindow& window) {
        const SceneViewport sv = window.getSceneViewport();
        return {static_cast<float>(sv.width), static_cast<float>(sv.height)};
    }

    glm::vec2 mouseInCanvas(const GameWindow& window) {
        const SceneViewport sv = window.getSceneViewport();
        auto [mx, my] = window.getMousePosition();
        return {static_cast<float>(mx - sv.x), static_cast<float>(my - sv.y)};
    }
} // namespace

std::shared_ptr<UIWindow> UISystem::createWindow() {
    auto window = std::make_shared<UIWindow>();
    window->zOrder = nextZOrder_++;
    windows_.push_back(window);
    return window;
}

void UISystem::destroyWindow(const std::shared_ptr<UIWindow>& window) {
    window->dead = true;
    std::erase(windows_, window);
}

void UISystem::clear() {
    for (auto& w: windows_)
        w->dead = true;
    windows_.clear();
    pressedButton_.reset();
    draggedWindow_.reset();
}

UISystem::HitResult UISystem::hitTest(glm::vec2 mouse, glm::vec2 canvasSize) const {
    // Topmost window first
    std::vector<std::shared_ptr<UIWindow>> sorted = windows_;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a->zOrder > b->zOrder; });

    for (const auto& win: sorted) {
        if (!win->visible) continue;
        const glm::vec2 winPos = win->resolvePos({0.0f, 0.0f}, canvasSize);
        if (!win->contains(winPos, mouse)) continue;

        HitResult hit;
        hit.window = win;
        for (const auto& child: win->children) {
            if (!child->visible) continue;
            auto button = std::dynamic_pointer_cast<UIButton>(child);
            if (!button) continue;
            const glm::vec2 pos = button->resolvePos(winPos, win->size);
            if (button->contains(pos, mouse)) {
                hit.button = button;
                break;
            }
        }
        return hit;
    }
    return {};
}

void UISystem::bringToFront(UIWindow* window) {
    if (window->zOrder != nextZOrder_ - 1) window->zOrder = nextZOrder_++;
}

void UISystem::update(InputSystem& input, const GameWindow& window) {
    for (const auto& win: windows_)
        for (const auto& child: win->children)
            if (auto* button = dynamic_cast<UIButton*>(child.get())) {
                button->hovered = false;
                button->pressed = false;
            }

    if (input.isBlocked()) {
        pressedButton_.reset();
        draggedWindow_.reset();
        input.setMouseCapturedByUI(false);
        return;
    }

    const glm::vec2 canvasSize = canvasSizeOf(window);
    const glm::vec2 mouse = mouseInCanvas(window);
    const HitResult hit = hitTest(mouse, canvasSize);

    const bool down = input.isMouseButtonDownRaw(0);
    const bool pressed = input.isMouseButtonPressedRaw(0);
    const bool released = !down && input.wasMouseButtonDownRaw(0);

    if (pressed && hit.window) {
        bringToFront(hit.window.get());
        if (hit.button) {
            pressedButton_ = hit.button;
        } else if (hit.window->draggable) {
            const glm::vec2 winPos = hit.window->resolvePos({0.0f, 0.0f}, canvasSize);
            const bool inTitleBar = !hit.window->hasTitleBar() || (mouse.y < winPos.y + UIWindow::TITLE_BAR_HEIGHT);
            if (inTitleBar) draggedWindow_ = hit.window;
        }
    }

    if (auto dragged = draggedWindow_.lock(); dragged && down) {
        dragged->offset += mouse - lastMouse_;
    } else {
        draggedWindow_.reset();
    }

    if (auto pressedBtn = pressedButton_.lock()) {
        if (down) {
            pressedBtn->pressed = (hit.button == pressedBtn);
        } else {
            if (released && hit.button == pressedBtn && !pressedBtn->dead && pressedBtn->onClick) pressedBtn->onClick();
            pressedButton_.reset();
        }
    }

    if (auto hoveredBtn = hit.button) hoveredBtn->hovered = true;

    input.setMouseCapturedByUI(hit.window != nullptr || !draggedWindow_.expired());
    lastMouse_ = mouse;
}

void UISystem::emitQuad(std::vector<UIDrawCall>& out,
                        const std::string& texture,
                        glm::vec2 pos,
                        glm::vec2 size,
                        glm::vec4 color,
                        glm::vec2 uv0,
                        glm::vec2 uv1) {
    if (size.x <= 0.0f || size.y <= 0.0f || color.a <= 0.0f) return;

    if (out.empty() || out.back().kind != UIDrawCall::Kind::Quads || out.back().textureOrFont != texture) {
        UIDrawCall call;
        call.kind = UIDrawCall::Kind::Quads;
        call.textureOrFont = texture;
        out.push_back(std::move(call));
    }
    auto& verts = out.back().vertices;

    const glm::vec3 tl{pos.x, pos.y, 0.0f};
    const glm::vec3 tr{pos.x + size.x, pos.y, 0.0f};
    const glm::vec3 bl{pos.x, pos.y + size.y, 0.0f};
    const glm::vec3 br{pos.x + size.x, pos.y + size.y, 0.0f};

    verts.push_back({tl, {uv0.x, uv0.y}, color});
    verts.push_back({bl, {uv0.x, uv1.y}, color});
    verts.push_back({tr, {uv1.x, uv0.y}, color});
    verts.push_back({tr, {uv1.x, uv0.y}, color});
    verts.push_back({bl, {uv0.x, uv1.y}, color});
    verts.push_back({br, {uv1.x, uv1.y}, color});
}

void UISystem::emitNineSlice(std::vector<UIDrawCall>& out,
                             const std::string& texture,
                             glm::vec2 pos,
                             glm::vec2 size,
                             float border,
                             glm::vec4 color) {
    const float b = std::min({border, size.x * 0.5f, size.y * 0.5f});
    // UV inset assumes the border occupies the same fraction on a square-ish
    // texture; using 0.33 keeps corners crisp for typical 3x3 frame textures.
    constexpr float UV_B = 1.0f / 3.0f;

    const float xs[4] = {0.0f, b, size.x - b, size.x};
    const float ys[4] = {0.0f, b, size.y - b, size.y};
    const float us[4] = {0.0f, UV_B, 1.0f - UV_B, 1.0f};
    const float vs[4] = {0.0f, UV_B, 1.0f - UV_B, 1.0f};

    for (int row = 0; row < 3; ++row)
        for (int col = 0; col < 3; ++col)
            emitQuad(out,
                     texture,
                     pos + glm::vec2{xs[col], ys[row]},
                     {xs[col + 1] - xs[col], ys[row + 1] - ys[row]},
                     color,
                     {us[col], vs[row]},
                     {us[col + 1], vs[row + 1]});
}

void UISystem::emitWindow(const UIWindow& win, glm::vec2 canvasSize, std::vector<UIDrawCall>& out) {
    const glm::vec2 winPos = win.resolvePos({0.0f, 0.0f}, canvasSize);

    if (!win.bgTexture.empty() && win.border > 0.0f)
        emitNineSlice(out, win.bgTexture, winPos, win.size, win.border, win.bgColor);
    else
        emitQuad(out, win.bgTexture, winPos, win.size, win.bgColor);

    if (win.hasTitleBar()) {
        emitQuad(out, "", winPos, {win.size.x, UIWindow::TITLE_BAR_HEIGHT}, win.titleBarColor);
        UIDrawCall title;
        title.kind = UIDrawCall::Kind::Text;
        title.text = win.title;
        title.position =
                winPos + glm::vec2{win.size.x * 0.5f, UIWindow::TITLE_BAR_HEIGHT * 0.5f + win.titleFontSize * 0.35f};
        title.fontSize = win.titleFontSize;
        title.color = win.titleColor;
        title.alignment = TextAlignment::Center;
        out.push_back(std::move(title));
    }

    for (const auto& child: win.children) {
        if (!child->visible) continue;
        const glm::vec2 pos = child->resolvePos(winPos, win.size);

        if (const auto* image = dynamic_cast<const UIImage*>(child.get())) {
            emitQuad(out, image->texture, pos, image->size, image->tint);
        } else if (const auto* label = dynamic_cast<const UILabel*>(child.get())) {
            UIDrawCall call;
            call.kind = UIDrawCall::Kind::Text;
            call.textureOrFont = label->fontName;
            call.text = label->text;
            // Baseline: anchor text vertically inside the widget rect
            call.position = pos + glm::vec2{0.0f, label->fontSize};
            call.fontSize = label->fontSize;
            call.color = label->color;
            call.alignment = label->alignment;
            out.push_back(std::move(call));
        } else if (const auto* button = dynamic_cast<const UIButton*>(child.get())) {
            const glm::vec4 bg = button->pressed   ? button->pressedColor
                                 : button->hovered ? button->hoverColor
                                                   : button->color;
            emitQuad(out, button->texture, pos, button->size, bg);
            if (!button->text.empty()) {
                UIDrawCall call;
                call.kind = UIDrawCall::Kind::Text;
                call.textureOrFont = button->fontName;
                call.text = button->text;
                call.position =
                        pos + glm::vec2{button->size.x * 0.5f, button->size.y * 0.5f + button->fontSize * 0.35f};
                call.fontSize = button->fontSize;
                call.color = button->textColor;
                call.alignment = TextAlignment::Center;
                out.push_back(std::move(call));
            }
        }
    }
}

void UISystem::buildDrawQueue(std::vector<UIDrawCall>& out, const GameWindow& window) const {
    const glm::vec2 canvasSize = canvasSizeOf(window);

    std::vector<std::shared_ptr<UIWindow>> sorted = windows_;
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a->zOrder < b->zOrder; });

    for (const auto& win: sorted) {
        if (!win->visible) continue;
        emitWindow(*win, canvasSize, out);
    }
}
