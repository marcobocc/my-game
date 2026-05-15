#pragma once
#include <imgui.h>
#include "../../../../business/EditorSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"
#include "../EditorPanel.hpp"
#include "GameObjectInspector.hpp"
#include "MaterialInspector.hpp"
#include "modules/scene/EntityManager.hpp"

class InspectorPanel : public EditorPanel {
public:
    InspectorPanel(EditorSelection& editorSelection,
                   EntityManager& entityManager,
                   EditorAssetRepository& assetRepository,
                   SceneMutations& sceneMutations,
                   MaterialMutations& materialMutations,
                   EditorGizmos& debugViz) :
        editorSelection_(editorSelection),
        gameObjectInspector_(editorSelection, entityManager, assetRepository, sceneMutations, debugViz),
        materialInspector(assetRepository, editorSelection, materialMutations) {}

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImVec2 position = {viewport->Pos.x + viewport->Size.x - width, viewport->Pos.y + menuBarHeight};
        ImVec2 size = {width, viewport->Size.y - menuBarHeight};
        EditorPanel::draw("Inspector", position, size);
    }

    void drawBody() override {
        const auto& entities = editorSelection_.getSelectedEntityIds();
        const auto& assets = editorSelection_.getSelectedAssetIds();
        if (entities.size() == 1) {
            gameObjectInspector_.draw(entities[0]);
        } else if (assets.size() == 1 && assets[0].ends_with(".mat")) {
            materialInspector.draw();
        }
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    EditorSelection& editorSelection_;
    GameObjectInspector gameObjectInspector_;
    MaterialInspector materialInspector;
};
