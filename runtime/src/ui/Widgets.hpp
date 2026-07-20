#pragma once
#include <functional>
#include <string>
#include "Widget.hpp"
#include "graphics/components/TextComponent.hpp"

class UILabel final : public Widget {
public:
    std::string text;
    std::string fontName; // empty = engine default font
    float fontSize = 16.0f;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    TextAlignment alignment = TextAlignment::Left;
};

class UIImage final : public Widget {
public:
    std::string texture;
    glm::vec4 tint{1.0f, 1.0f, 1.0f, 1.0f};
};

class UIButton final : public Widget {
public:
    std::string text;
    std::string fontName;
    float fontSize = 16.0f;
    std::string texture; // empty = flat colored quad
    glm::vec4 textColor{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 color{0.25f, 0.25f, 0.28f, 1.0f};
    glm::vec4 hoverColor{0.35f, 0.35f, 0.40f, 1.0f};
    glm::vec4 pressedColor{0.15f, 0.15f, 0.18f, 1.0f};
    std::function<void()> onClick;

    bool hovered = false;
    bool pressed = false;
};

class UIWindow final : public Widget {
public:
    static constexpr float TITLE_BAR_HEIGHT = 28.0f;

    std::string title;
    std::string bgTexture; // empty = flat colored quad
    float border = 0.0f; // 9-slice inset in pixels (0 = stretch whole texture)
    glm::vec4 bgColor{0.10f, 0.10f, 0.12f, 0.95f};
    glm::vec4 titleBarColor{0.05f, 0.05f, 0.06f, 1.0f};
    glm::vec4 titleColor{1.0f, 1.0f, 1.0f, 1.0f};
    float titleFontSize = 16.0f;
    bool draggable = false;
    int zOrder = 0;

    bool hasTitleBar() const { return !title.empty(); }
};
