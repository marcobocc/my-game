#pragma once
#include <imgui.h>
#include "../../../../business/ObjectSelection.hpp"
#include "../../../../business/asset_editing/EditorAssetRepository.hpp"
#include "../../../../business/asset_editing/MaterialMutations.hpp"
#include "../EditorPanel.hpp"
#include "GameObjectInspector.hpp"
#include "MaterialInspector.hpp"
#include "modules/scene/EntityManager.hpp"

class InspectorPanel : public EditorPanel {
public:
    InspectorPanel(ObjectSelection& objectSelection,
                   EntityManager& entityManager,
                   EditorAssetRepository& assetRepository,
                   SceneMutations& sceneMutations,
                   MaterialMutations& materialMutations,
                   EditorGizmos& debugViz) :
        objectSelection_(objectSelection),
        gameObjectInspector_(objectSelection, entityManager, assetRepository, sceneMutations, debugViz),
        materialInspector(assetRepository, objectSelection, materialMutations) {}

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImVec2 position = {viewport->Pos.x + viewport->Size.x - width, viewport->Pos.y + menuBarHeight};
        ImVec2 size = {width, viewport->Size.y - menuBarHeight};
        EditorPanel::draw("Inspector", position, size);
    }

    void drawBody() override {
        if (objectSelection_.isElementGameObject()) {
            auto selectedId = objectSelection_.getSelectedEntityId();
            gameObjectInspector_.draw(*selectedId);
        } else if (objectSelection_.isElementMaterial()) {
            auto selectedAsset = objectSelection_.getSelectedAssetId();
            materialInspector.draw();
        }
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    ObjectSelection& objectSelection_;
    GameObjectInspector gameObjectInspector_;
    MaterialInspector materialInspector;
};
