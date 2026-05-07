#pragma once
#include <imgui.h>
#include <optional>
#include <string>
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/scene_editing/SceneMutations.hpp"

class SceneMutations;
#include "../ui_components/hierarchy_panel/HierarchyDropdownMenu.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/scene/EntityManager.hpp"
#include "modules/scene/EntityMetadata.hpp"
#include "modules/ui/ImguiWidget.hpp"

class HierarchyPanel : public ImguiWidget {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;

    HierarchyPanel(ObjectSelection& objectSelection,
                   EntityManager& entityManager,
                   AssetManager& assetManager,
                   SceneMutations& sceneMutations) :
        objectSelection_(objectSelection),
        entityManager_(entityManager),
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

        auto selectedId = objectSelection_.getSelectedEntityId();

        for (EntityHandle e: entityManager_.getEntities()) {
            std::string label;
            if (const auto* meta = entityManager_.getMetadata(e)) {
                label = meta->entityName.empty() ? "Entity " + std::to_string(e) : meta->entityName;
            } else {
                label = "Entity " + std::to_string(e);
            }

            bool selected = selectedId && *selectedId == e;
            if (ImGui::Selectable(("##entity" + std::to_string(e)).c_str(),
                                  selected,
                                  ImGuiSelectableFlags_AllowOverlap,
                                  {0, 0})) {
                objectSelection_.selectObject(e);
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                contextTargetId.emplace(e);
                openContextMenu = true;
            }
            ImGui::SameLine();
            ImGui::Text("%s", label.c_str());
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
    EntityManager& entityManager_;
    AssetManager& assetManager_;
    SceneMutations& sceneMutations_;
    HierarchyDropdownMenu dropdownMenu_;

    std::optional<EntityHandle> contextTargetId;
    bool openContextMenu = false;
};
