#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/SceneMutations.hpp"

class SceneMutations;
#include "../ui_components/hierarchy_panel/HierarchyDropdownMenu.hpp"
#include "systems/assets/AssetManager.hpp"
#include "systems/scene/Scene.hpp"
#include "systems/ui/ImguiWidget.hpp"

class HierarchyPanel : public ImguiWidget {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;

    HierarchyPanel(ObjectSelection& objectSelection,
                   Scene& scene,
                   AssetManager& assetManager,
                   SceneMutations& sceneMutations) :
        objectSelection_(objectSelection),
        scene_(scene),
        assetManager_(assetManager),
        sceneMutations_(sceneMutations),
        dropdownMenu_(assetManager, sceneMutations) {}

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
            ImGui::PushID(id.c_str());
            auto selectedId = objectSelection_.getSelectedObjectId();
            bool isSelected = selectedId.has_value() && *selectedId == id;
            if (ImGui::Selectable(id.c_str(), isSelected)) {
                objectSelection_.selectObject(id);
            }
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                contextTargetId = id;
                openContextMenu = true;
            }
            ImGui::PopID();
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
            contextTargetId.reset();
            openContextMenu = true;
        }

        if (openContextMenu) {
            ImGui::OpenPopup("HierarchyContextMenu");
            openContextMenu = false;
        }

        if (ImGui::BeginPopup("HierarchyContextMenu")) {
            dropdownMenu_.draw(contextTargetId);
            ImGui::EndPopup();
        }

        ImGui::End();
    }

private:
    ObjectSelection& objectSelection_;
    Scene& scene_;
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    HierarchyDropdownMenu dropdownMenu_;

    std::optional<std::string> contextTargetId;
    bool openContextMenu = false;
};
