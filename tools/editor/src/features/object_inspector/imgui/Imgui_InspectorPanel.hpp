#pragma once
#include <imgui.h>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "Imgui_GameObjectInspector.hpp"
#include "assets/Imgui_MaterialWidget.hpp"

class Imgui_InspectorPanel : public EditorPanel {
public:
    Imgui_InspectorPanel(EditorSelection& editorSelection,
                         AssetStore& assetStore,
                         RuntimeScene& scene,
                         EditorGizmos& debugViz) :
        editorSelection_(editorSelection),
        gameObjectInspector_(editorSelection, assetStore, scene, debugViz),
        materialInspector(assetStore, editorSelection) {}

    void draw() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight();
        float width = viewport->Size.x * PANEL_WIDTH_RATIO;
        ImVec2 position = {viewport->Pos.x + viewport->Size.x - width, viewport->Pos.y + menuBarHeight};
        ImVec2 size = {width, viewport->Size.y - menuBarHeight};
        EditorPanel::draw("Inspector", position, size);
    }

    void drawBody() override {
        const auto& inspectedEntity = editorSelection_.getInspectedEntity();
        const auto& inspectedAsset = editorSelection_.getInspectedAsset();
        if (inspectedAsset.has_value()) {
            if (inspectedAsset->ends_with(".mat")) {
                materialInspector.draw();
            }
        } else if (inspectedEntity.has_value()) {
            gameObjectInspector_.draw(*inspectedEntity);
        }
    }

    static constexpr float PANEL_WIDTH_RATIO = 0.25f;

private:
    EditorSelection& editorSelection_;
    Imgui_GameObjectInspector gameObjectInspector_;
    Imgui_MaterialWidget materialInspector;
};
