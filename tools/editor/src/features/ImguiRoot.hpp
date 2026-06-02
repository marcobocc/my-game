#pragma once
#include "../../../../runtime/src/modules/console/Imgui_Console.hpp"
#include "application_toolbar/Imgui_ApplicationMenuBar.hpp"
#include "asset_browser/imgui/Imgui_AssetExplorerPanel.hpp"
#include "object_inspector/imgui/Imgui_InspectorPanel.hpp"
#include "scene_hierarchy/imgui/Imgui_HierarchyPanel.hpp"
#include "scene_viewport/imgui/Imgui_EditorSceneViewport.hpp"
#include "scene_viewport/scene_toolbar/Imgui_SceneViewToolbar.hpp"

class ImguiRoot {
public:
    ImguiRoot(Imgui_ApplicationMenuBar& applicationMenuBar,
              Imgui_SceneViewToolbar& sceneViewToolbar,
              Imgui_HierarchyPanel& hierarchyPanel,
              Imgui_AssetExplorerPanel& assetExplorerPanel,
              Imgui_InspectorPanel& inspectorPanel,
              Imgui_EditorSceneViewport& editorSceneViewport,
              Imgui_Console& console) :
        applicationMenuBar_(applicationMenuBar),
        sceneViewToolbar_(sceneViewToolbar),
        hierarchyPanel_(hierarchyPanel),
        assetExplorerPanel_(assetExplorerPanel),
        inspectorPanel_(inspectorPanel),
        editorSceneViewport_(editorSceneViewport),
        console_(console) {}

    void draw() {
        applicationMenuBar_.draw();
        sceneViewToolbar_.draw();
        hierarchyPanel_.draw();
        assetExplorerPanel_.draw();
        inspectorPanel_.draw();
        editorSceneViewport_.draw();
        console_.draw();
    }

private:
    Imgui_ApplicationMenuBar& applicationMenuBar_;
    Imgui_SceneViewToolbar& sceneViewToolbar_;
    Imgui_HierarchyPanel& hierarchyPanel_;
    Imgui_AssetExplorerPanel& assetExplorerPanel_;
    Imgui_InspectorPanel& inspectorPanel_;
    Imgui_EditorSceneViewport& editorSceneViewport_;
    Imgui_Console& console_;
};
