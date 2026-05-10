#pragma once
#include <imgui.h>
#include <string>
#include "../../../../business/ObjectSelection.hpp"
#include "../../ImguiStyling.hpp"
#include "../EditorPanel.hpp"
#include "HierarchyDropdownMenu.hpp"
#include "modules/assets/AssetManager.hpp"
#include "modules/scene/EntityManager.hpp"
#include "modules/scene/EntityMetadata.hpp"

class SceneMutations;

class HierarchyPanel : public EditorPanel {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;
    static constexpr float ASSETS_HEIGHT_RATIO = 0.3f;

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
        float height = viewport->Size.y - menuBarHeight - (viewport->Size.y * ASSETS_HEIGHT_RATIO);

        ImVec2 position = {viewport->Pos.x, viewport->Pos.y + menuBarHeight};
        ImVec2 size = {width, height};
        EditorPanel::draw("Objects Hierarchy", position, size);
    }

    void drawBody() override {
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

        ImguiStyling::withPopup("HierarchyContextMenu", [&] { dropdownMenu_.draw(contextTargetId); });
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
