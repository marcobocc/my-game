#pragma once
#include <imgui.h>
#include <string>
#include "../../../../business/ObjectSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/scene_editing/ObjectPrefabs.hpp"
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
                   ObjectSelection& objectSelection,
                   EntityManager& entityManager,
                   SceneMutations& sceneMutations,
                   GameEngine& engine) :
        repository_(repository),
        objectSelection_(objectSelection),
        entityManager_(entityManager),
        sceneMutations_(sceneMutations),
        engine_(engine),
        spherePopupModal_([this](uint32_t lod) { sceneMutations_.createObject(primitives::sphere(lod)); }),
        dropdownMenu_(repository, sceneMutations, &spherePopupModal_) {}

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
        auto selectedId = objectSelection_.getSelectedEntityId();
        ImGui::Separator();

        bool rightClickedThisFrame = false;
        std::optional<EntityHandle> rightClickedEntity;

        for (EntityHandle e: entityManager_.getEntities()) {
            std::string label;
            if (const auto* meta = entityManager_.getMetadata(e)) {
                label = meta->entityName.empty() ? "Entity " + std::to_string(e) : meta->entityName;
            } else {
                label = "Entity " + std::to_string(e);
            }

            bool selected = selectedId && *selectedId == e;
            bool isContextTarget = contextTargetId && *contextTargetId == e;
            if (ImGui::Selectable(("##entity" + std::to_string(e)).c_str(),
                                  selected,
                                  ImGuiSelectableFlags_AllowOverlap,
                                  {0, 0})) {
                objectSelection_.selectObject(e);
            }
            bool itemHovered = ImGui::IsItemHovered() || isContextTarget;
            ImGui::SameLine();
            ImGui::Text("%s", label.c_str());
            itemHovered = itemHovered || ImGui::IsItemHovered();

            if (itemHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
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
    ObjectSelection& objectSelection_;
    EntityManager& entityManager_;
    SceneMutations& sceneMutations_;
    GameEngine& engine_;
    SpherePopupModal spherePopupModal_;
    HierarchyDropdownMenu dropdownMenu_;

    std::optional<EntityHandle> contextTargetId;
};
