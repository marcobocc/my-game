#pragma once
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <string>
#include "../../../../business/ActionDispatcher.hpp"
#include "../../../../business/EditorSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/scene_editing/ObjectBuilder.hpp"
#include "../../../../input/ShortcutBindingService.hpp"
#include "../../ImguiStyling.hpp"
#include "../EditorPanel.hpp"
#include "HierarchyDropdownMenu.hpp"
#include "SpherePopupModal.hpp"
#include "modules/scene/EntityManager.hpp"
#include "modules/scene/EntityMetadata.hpp"

class SceneMutations;

class HierarchyPanel : public EditorPanel {
public:
    static constexpr float PANEL_WIDTH_RATIO = 0.15f;
    static constexpr float ASSETS_HEIGHT_RATIO = 0.3f;

    HierarchyPanel(EditorAssetRepository& repository,
                   EditorSelection& editorSelection,
                   EntityManager& entityManager,
                   SceneMutations& sceneMutations,
                   ObjectBuilder& objectBuilder,
                   GameEngine& engine,
                   ActionDispatcher& actionDispatcher,
                   ShortcutBindingService& shortcutBindingService) :
        repository_(repository),
        editorSelection_(editorSelection),
        entityManager_(entityManager),
        sceneMutations_(sceneMutations),
        objectBuilder_(objectBuilder),
        engine_(engine),
        spherePopupModal_(sceneMutations_, objectBuilder_),
        dropdownMenu_(repository,
                      sceneMutations,
                      objectBuilder,
                      actionDispatcher,
                      shortcutBindingService,
                      &spherePopupModal_) {}

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        float assetsHeight = viewport->Size.y * ASSETS_HEIGHT_RATIO;
        float height = viewport->Size.y - menuBarHeight - assetsHeight;

        ImVec2 position = {viewport->Pos.x, viewport->Pos.y + menuBarHeight};
        ImVec2 size = {width, height};
        EditorPanel::draw("Objects Hierarchy", position, size);
    }

    void drawBody() override {
        contextTargetId.reset();
        spherePopupModal_.draw();
        ImGui::Separator();

        bool cmdDown = ImGui::GetIO().KeySuper || ImGui::GetIO().KeyCtrl;
        bool rightClickedThisFrame = false;
        std::optional<EntityHandle> rightClickedEntity;

        for (EntityHandle e: entityManager_.getEntities()) {
            std::string label;
            if (const auto* meta = entityManager_.getMetadata(e)) {
                label = meta->entityName.empty() ? "Entity " + std::to_string(e) : meta->entityName;
            } else {
                label = "Entity " + std::to_string(e);
            }

            bool selected = editorSelection_.isEntitySelected(e);
            bool isContextTarget = contextTargetId && *contextTargetId == e;
            ImGui::Selectable(
                    ("##entity" + std::to_string(e)).c_str(), selected, ImGuiSelectableFlags_AllowOverlap, {0, 0});
            bool hovered = ImGui::IsItemHovered() || isContextTarget;
            ImGui::SameLine();
            ImGui::TextUnformatted(label.c_str());
            hovered = hovered || ImGui::IsItemHovered();

            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                if (cmdDown) {
                    if (editorSelection_.isEntitySelected(e)) {
                        editorSelection_.removeFromSelection(e);
                    } else {
                        editorSelection_.addToSelection(e);
                    }
                } else {
                    editorSelection_.clearSelection();
                    editorSelection_.addToSelection(e);
                }
            }
            if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                rightClickedThisFrame = true;
                rightClickedEntity.emplace(e);
            }
        }

        if (!rightClickedThisFrame && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            rightClickedThisFrame = true;
            rightClickedEntity.reset();
        }

        if (rightClickedThisFrame) {
            contextTargetId = rightClickedEntity;
            ImGui::OpenPopup("HierarchyContextMenu");
        }

        ImguiStyling::withPopup("HierarchyContextMenu", [&] { dropdownMenu_.draw(contextTargetId); }, true);
    }

private:
    EditorAssetRepository& repository_;
    EditorSelection& editorSelection_;
    EntityManager& entityManager_;
    SceneMutations& sceneMutations_;
    ObjectBuilder& objectBuilder_;
    GameEngine& engine_;
    SpherePopupModal spherePopupModal_;
    HierarchyDropdownMenu dropdownMenu_;

    std::optional<EntityHandle> contextTargetId;
};
