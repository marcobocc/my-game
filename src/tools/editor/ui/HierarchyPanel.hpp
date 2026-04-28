#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "assets/AssetManager.hpp"
#include "core/scene/Scene.hpp"
#include "core/ui/ImguiWidget.hpp"

class HierarchyPanel : public ImguiWidget {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;

    HierarchyPanel(std::optional<std::string>* selectedObjectId, Scene* scene, AssetManager* assetManager) :
        selectedObjectId_(selectedObjectId),
        scene_(scene),
        assetManager_(assetManager) {}

    void draw() const override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImGui::SetNextWindowPos({viewport->Pos.x, viewport->Pos.y + menuBarHeight});
        ImGui::SetNextWindowSize({width, viewport->Size.y - menuBarHeight});
        ImGui::SetNextWindowBgAlpha(0.95f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin("Hierarchy", nullptr, flags);

        ImGui::Spacing();
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Objects Hierarchy");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (const auto& [id, obj]: scene_->getObjects()) {
            bool isSelected = selectedObjectId_->has_value() && **selectedObjectId_ == id;
            if (ImGui::Selectable(id.c_str(), isSelected))
                *selectedObjectId_ = (isSelected) ? std::nullopt : std::optional<std::string>(id);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        drawAddObject();

        ImGui::End();
    }

private:
    std::optional<std::string>* selectedObjectId_;
    Scene* scene_;
    AssetManager* assetManager_;

    void drawAddObject() const {
        if (!ImGui::CollapsingHeader("Add Object")) return;

        ImGui::Indent();
        if (ImGui::CollapsingHeader("Primitives")) {
            if (ImGui::Selectable("  Cube")) {
                auto id = scene_->createCube({});
                *selectedObjectId_ = id;
            }
            if (ImGui::Selectable("  Rectangle 2D")) {
                auto id = scene_->createRectangle2D({});
                *selectedObjectId_ = id;
            }
        }

        if (ImGui::CollapsingHeader("Models")) {
            auto models = assetManager_->getAvailableAssets(".model");
            if (models.empty()) {
                ImGui::TextDisabled("  No models found");
            } else {
                for (const auto& modelName: models) {
                    if (ImGui::Selectable(("  " + modelName).c_str())) {
                        auto id = scene_->createModel(modelName, {});
                        *selectedObjectId_ = id;
                    }
                }
            }
        }
        ImGui::Unindent();
    }
};
