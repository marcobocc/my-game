#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

enum class UIAnchor { TopLeft, Top, TopRight, Left, Center, Right, BottomLeft, Bottom, BottomRight };

inline glm::vec2 anchorFactor(UIAnchor anchor) {
    switch (anchor) {
        case UIAnchor::TopLeft:
            return {0.0f, 0.0f};
        case UIAnchor::Top:
            return {0.5f, 0.0f};
        case UIAnchor::TopRight:
            return {1.0f, 0.0f};
        case UIAnchor::Left:
            return {0.0f, 0.5f};
        case UIAnchor::Center:
            return {0.5f, 0.5f};
        case UIAnchor::Right:
            return {1.0f, 0.5f};
        case UIAnchor::BottomLeft:
            return {0.0f, 1.0f};
        case UIAnchor::Bottom:
            return {0.5f, 1.0f};
        case UIAnchor::BottomRight:
            return {1.0f, 1.0f};
    }
    return {0.0f, 0.0f};
}

class Widget {
public:
    virtual ~Widget() = default;

    UIAnchor anchor = UIAnchor::TopLeft;
    glm::vec2 offset{0.0f};
    glm::vec2 size{0.0f};
    bool visible = true;
    bool dead = false;
    std::vector<std::shared_ptr<Widget>> children;

    // Top-left corner in canvas coordinates. The widget aligns its own
    // anchor-matching corner to the parent's anchor point, then applies offset.
    glm::vec2 resolvePos(glm::vec2 parentPos, glm::vec2 parentSize) const {
        const glm::vec2 f = anchorFactor(anchor);
        return parentPos + parentSize * f - size * f + offset;
    }

    bool contains(glm::vec2 pos, glm::vec2 point) const {
        return point.x >= pos.x && point.x < pos.x + size.x && point.y >= pos.y && point.y < pos.y + size.y;
    }
};
