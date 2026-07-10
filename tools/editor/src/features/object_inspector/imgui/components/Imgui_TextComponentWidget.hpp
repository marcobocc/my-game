#pragma once
#include <array>
#include <cstring>
#include <optional>
#include "../../../../../../../runtime/src/graphics/components/TextComponent.hpp"
#include "../Imgui_InspectorWidget.hpp"

class RuntimeScene;

class Imgui_TextComponentWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_TextComponentWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Text Component"), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const TextComponent& c) {
        if (isEditing_) return;
        component_ = c;
        std::strncpy(textBuf_.data(), c.text.c_str(), textBuf_.size() - 1);
        textBuf_[textBuf_.size() - 1] = '\0';
    }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    TextComponent component_{};
    std::array<char, 256> textBuf_{};
    bool isEditing_ = false;

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<InputTextEditableProperty>("Text", textBuf_, [this] {
            component_.text = textBuf_.data();
            commit(UndoHistory::randomGroupId("Set Text"));
        });

        add<ColorEdit4Property>(
                "Color",
                [this] { return component_.color; },
                [this](glm::vec4 v) { component_.color = v; },
                [this] { isEditing_ = true; },
                [this] {
                    isEditing_ = false;
                    commit(UndoHistory::randomGroupId("Set Color"));
                });

        add<DragFloatProperty>(
                "Font Size",
                [this] { return component_.fontSize; },
                [this](float v) { component_.fontSize = v; },
                0.5f,
                0.0f,
                100.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Font Size")); });

        add<CheckboxProperty>(
                "Visible",
                [this] { return component_.visible; },
                [this](bool v) {
                    component_.visible = v;
                    commit(UndoHistory::randomGroupId("Set Visible"));
                });

        add<CheckboxProperty>(
                "Billboard",
                [this] { return component_.billboard; },
                [this](bool v) {
                    component_.billboard = v;
                    commit(UndoHistory::randomGroupId("Set Billboard"));
                });

        add<ComboProperty>(
                "Alignment",
                std::vector<std::string>{"Left", "Center", "Right"},
                [this] { return static_cast<int>(component_.alignment); },
                [this](int v) {
                    component_.alignment = static_cast<TextAlignment>(v);
                    commit(UndoHistory::randomGroupId("Set Alignment"));
                });
    }

    void commit(const std::string& groupId) {
        if (!lastObjectId_) return;
        TextComponent snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<TextComponent>([snapshot](TextComponent& c) { c = snapshot; }, groupId);
    }
};
