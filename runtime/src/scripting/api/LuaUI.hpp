#pragma once
#include <log4cxx/logger.h>
#include <memory>
#include <sol/sol.hpp>
#include <string>
#include "ui/UISystem.hpp"

/*
    LuaUI

    High-level Lua facade over UISystem. Scripts build UI declaratively
    (windows, buttons, labels, images) and register callbacks; no per-frame
    polling of widget state from Lua.
*/

namespace LuaUIDetail {

    inline UIAnchor parseAnchor(const std::string& name) {
        if (name == "Top") return UIAnchor::Top;
        if (name == "TopRight") return UIAnchor::TopRight;
        if (name == "Left") return UIAnchor::Left;
        if (name == "Center") return UIAnchor::Center;
        if (name == "Right") return UIAnchor::Right;
        if (name == "BottomLeft") return UIAnchor::BottomLeft;
        if (name == "Bottom") return UIAnchor::Bottom;
        if (name == "BottomRight") return UIAnchor::BottomRight;
        return UIAnchor::TopLeft;
    }

    inline glm::vec4 parseColor(const sol::table& args, const char* key, glm::vec4 fallback) {
        sol::optional<sol::table> t = args[key];
        if (!t) return fallback;
        return {t->get_or(1, fallback.r), t->get_or(2, fallback.g), t->get_or(3, fallback.b), t->get_or(4, fallback.a)};
    }

    inline void applyCommon(Widget& w, const sol::table& args) {
        w.offset = {args.get_or("x", 0.0f), args.get_or("y", 0.0f)};
        w.size = {args.get_or("w", w.size.x), args.get_or("h", w.size.y)};
        w.anchor = parseAnchor(args.get_or<std::string>("anchor", "TopLeft"));
        w.visible = args.get_or("visible", true);
    }

} // namespace LuaUIDetail

class LuaUIButton {
public:
    explicit LuaUIButton(std::shared_ptr<UIButton> button) : button_(std::move(button)) {}

    void onClick(const sol::protected_function& fn) {
        button_->onClick = [fn]() {
            auto result = fn();
            if (!result.valid()) {
                sol::error err = result;
                LOG4CXX_ERROR(log4cxx::Logger::getLogger("LuaUI"), "onClick error: " << err.what());
            }
        };
    }

    void setText(const std::string& text) { button_->text = text; }
    void setVisible(bool visible) { button_->visible = visible; }

private:
    std::shared_ptr<UIButton> button_;
};

class LuaUILabel {
public:
    explicit LuaUILabel(std::shared_ptr<UILabel> label) : label_(std::move(label)) {}

    void setText(const std::string& text) { label_->text = text; }
    void setColor(float r, float g, float b, float a) { label_->color = {r, g, b, a}; }
    void setVisible(bool visible) { label_->visible = visible; }

private:
    std::shared_ptr<UILabel> label_;
};

class LuaUIImage {
public:
    explicit LuaUIImage(std::shared_ptr<UIImage> image) : image_(std::move(image)) {}

    void setTexture(const std::string& texture) { image_->texture = texture; }
    void setVisible(bool visible) { image_->visible = visible; }

private:
    std::shared_ptr<UIImage> image_;
};

class LuaUIWindow {
public:
    LuaUIWindow(UISystem& system, std::shared_ptr<UIWindow> window) : system_(&system), window_(std::move(window)) {}

    LuaUIButton addButton(const sol::table& args) {
        auto button = std::make_shared<UIButton>();
        button->size = {120.0f, 32.0f};
        LuaUIDetail::applyCommon(*button, args);
        button->text = args.get_or<std::string>("text", "");
        button->fontName = args.get_or<std::string>("font", "");
        button->fontSize = args.get_or("fontSize", button->fontSize);
        button->texture = args.get_or<std::string>("texture", "");
        button->color = LuaUIDetail::parseColor(args, "color", button->color);
        button->hoverColor = LuaUIDetail::parseColor(args, "hoverColor", button->hoverColor);
        button->pressedColor = LuaUIDetail::parseColor(args, "pressedColor", button->pressedColor);
        button->textColor = LuaUIDetail::parseColor(args, "textColor", button->textColor);
        window_->children.push_back(button);
        sol::optional<sol::protected_function> fn = args["onClick"];
        LuaUIButton handle(button);
        if (fn) handle.onClick(*fn);
        return handle;
    }

    LuaUILabel addLabel(const sol::table& args) {
        auto label = std::make_shared<UILabel>();
        LuaUIDetail::applyCommon(*label, args);
        label->text = args.get_or<std::string>("text", "");
        label->fontName = args.get_or<std::string>("font", "");
        label->fontSize = args.get_or("fontSize", label->fontSize);
        label->color = LuaUIDetail::parseColor(args, "color", label->color);
        const std::string align = args.get_or<std::string>("align", "Left");
        label->alignment = align == "Center"  ? TextAlignment::Center
                           : align == "Right" ? TextAlignment::Right
                                              : TextAlignment::Left;
        window_->children.push_back(label);
        return LuaUILabel(label);
    }

    LuaUIImage addImage(const sol::table& args) {
        auto image = std::make_shared<UIImage>();
        LuaUIDetail::applyCommon(*image, args);
        image->texture = args.get_or<std::string>("texture", "");
        image->tint = LuaUIDetail::parseColor(args, "tint", image->tint);
        window_->children.push_back(image);
        return LuaUIImage(image);
    }

    void show() { window_->visible = true; }
    void hide() { window_->visible = false; }
    bool isVisible() const { return window_->visible; }
    void setTitle(const std::string& title) { window_->title = title; }
    void destroy() { system_->destroyWindow(window_); }

private:
    UISystem* system_;
    std::shared_ptr<UIWindow> window_;
};

class LuaUI {
public:
    explicit LuaUI(UISystem& system) : system_(system) {}

    LuaUIWindow createWindow(const sol::table& args) {
        auto window = system_.createWindow();
        window->size = {300.0f, 200.0f};
        LuaUIDetail::applyCommon(*window, args);
        window->title = args.get_or<std::string>("title", "");
        window->bgTexture = args.get_or<std::string>("bg", "");
        window->border = args.get_or("border", 0.0f);
        window->bgColor = LuaUIDetail::parseColor(args, "bgColor", window->bgColor);
        window->titleBarColor = LuaUIDetail::parseColor(args, "titleBarColor", window->titleBarColor);
        window->draggable = args.get_or("draggable", false);
        return {system_, window};
    }

    void clear() { system_.clear(); }

private:
    UISystem& system_;
};

namespace LuaUIDetail {

    inline void registerTypes(sol::state& lua, LuaUI& ui) {
        lua.new_usertype<LuaUIButton>("UIButton",
                                      "onClick",
                                      &LuaUIButton::onClick,
                                      "setText",
                                      &LuaUIButton::setText,
                                      "setVisible",
                                      &LuaUIButton::setVisible);

        lua.new_usertype<LuaUILabel>("UILabel",
                                     "setText",
                                     &LuaUILabel::setText,
                                     "setColor",
                                     &LuaUILabel::setColor,
                                     "setVisible",
                                     &LuaUILabel::setVisible);

        lua.new_usertype<LuaUIImage>(
                "UIImage", "setTexture", &LuaUIImage::setTexture, "setVisible", &LuaUIImage::setVisible);

        lua.new_usertype<LuaUIWindow>("UIWindow",
                                      "addButton",
                                      &LuaUIWindow::addButton,
                                      "addLabel",
                                      &LuaUIWindow::addLabel,
                                      "addImage",
                                      &LuaUIWindow::addImage,
                                      "show",
                                      &LuaUIWindow::show,
                                      "hide",
                                      &LuaUIWindow::hide,
                                      "isVisible",
                                      &LuaUIWindow::isVisible,
                                      "setTitle",
                                      &LuaUIWindow::setTitle,
                                      "destroy",
                                      &LuaUIWindow::destroy);

        lua.new_usertype<LuaUI>("LuaUI", "createWindow", &LuaUI::createWindow, "clear", &LuaUI::clear);

        lua["UI"] = &ui;
    }

} // namespace LuaUIDetail
