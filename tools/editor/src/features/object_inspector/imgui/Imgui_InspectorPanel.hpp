#pragma once
#include <imgui.h>
#include "../../../services/EditorSelection.hpp"
#include "../../../services/common_editing/RuntimeScene.hpp"
#include "../../../styling/EditorPanel.hpp"
#include "Imgui_GameObjectInspector.hpp"
#include "assets/Imgui_MaterialWidget.hpp"
#include "assets/Imgui_MeshWidget.hpp"

class Imgui_InspectorPanel : public EditorPanel {
public:
    Imgui_InspectorPanel(EditorSelection& editorSelection,
                         AssetStore& assetStore,
                         RuntimeScene& scene,
                         EditorGizmos& debugViz) :
        editorSelection_(editorSelection),
        gameObjectInspector_(editorSelection, assetStore, scene, debugViz),
        materialInspector(assetStore, editorSelection),
        meshInspector(assetStore, editorSelection) {}

    void draw() { EditorPanel::draw("Inspector"); }

    void drawBody() override {
        const auto& inspectedEntity = editorSelection_.getInspectedEntity();
        const auto& inspectedAsset = editorSelection_.getInspectedAsset();
        if (inspectedAsset.has_value()) {
            if (inspectedAsset->ends_with(".mat")) {
                materialInspector.draw();
            } else if (inspectedAsset->ends_with(".mesh")) {
                meshInspector.draw();
            }
        } else if (inspectedEntity.has_value()) {
            gameObjectInspector_.draw(*inspectedEntity);
        }
    }

private:
    EditorSelection& editorSelection_;
    Imgui_GameObjectInspector gameObjectInspector_;
    Imgui_MaterialWidget materialInspector;
    Imgui_MeshWidget meshInspector;
};
