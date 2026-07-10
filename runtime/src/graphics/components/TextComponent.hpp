#pragma once
#include <glm/glm.hpp>
#include <string>
#include "../../core/components/IComponent.hpp"
#include "utils/JsonUtils.hpp"

enum class TextAlignment { Left = 0, Center = 1, Right = 2 };

struct TextComponent final : IComponent {
    std::string text;
    std::string fontName;
    glm::vec4 color{1.0f, 1.0f, 1.0f, 1.0f};
    float fontSize = 14.0f;
    bool visible = true;
    bool billboard = false;
    TextAlignment alignment = TextAlignment::Left;

    TextComponent() = default;

    std::string typeName() const override { return "TextComponent"; }

    nlohmann::json serialize() const override {
        return {{"text", text},
                {"fontName", fontName},
                {"color", {color.r, color.g, color.b, color.a}},
                {"fontSize", fontSize},
                {"visible", visible},
                {"billboard", billboard},
                {"alignment", static_cast<int>(alignment)}};
    }

    std::unique_ptr<IComponent> clone() const override { return std::make_unique<TextComponent>(*this); }

    static TextComponent deserialize(const nlohmann::json& j) {
        TextComponent c{};
        c.text = JsonUtils::getOptional<std::string>(j, "text", "");
        c.fontName = JsonUtils::getOptional<std::string>(j, "fontName", "");
        if (j.contains("color") && j["color"].is_array() && j["color"].size() == 4)
            c.color = {j["color"][0], j["color"][1], j["color"][2], j["color"][3]};
        c.fontSize = JsonUtils::getOptional<float>(j, "fontSize", 14.0f);
        c.visible = JsonUtils::getOptional<bool>(j, "visible", true);
        c.billboard = JsonUtils::getOptional<bool>(j, "billboard", false);
        c.alignment = static_cast<TextAlignment>(JsonUtils::getOptional<int>(j, "alignment", 0));
        return c;
    }
};
