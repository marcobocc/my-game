#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../controller/SceneMutationsController.hpp"
#include "../../../state/EditorState.hpp"
#include "HierarchyDropdownMenu.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/scene/Scene.hpp"
#include "systems/ui/ImguiWidget.hpp"

class HierarchyPanel : public ImguiWidget {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;

    HierarchyPanel(EditorState& editorState,
                   Scene& scene,
                   AssetManager& assetManager,
                   SceneMutationsController& mutations) :
        editorState_(editorState),
        scene_(scene),
        assetManager_(assetManager),
        mutations_(mutations),
        dropdownMenu_(editorState, scene, assetManager, mutations) {}

    void draw() override {
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

        for (const auto& [id, obj]: scene_.getObjects()) {
            bool isSelected = editorState_.getSelectedObject().has_value() && *editorState_.getSelectedObject() == id;
            if (ImGui::Selectable(id.c_str(), isSelected))
                editorState_.setSelectedObject((isSelected) ? std::nullopt : std::optional<std::string>(id));
        }

        dropdownMenu_.draw();

        ImGui::End();
    }

private:
    EditorState& editorState_;
    Scene& scene_;
    AssetManager& assetManager_;
    SceneMutationsController& mutations_;
    HierarchyDropdownMenu dropdownMenu_;
};
