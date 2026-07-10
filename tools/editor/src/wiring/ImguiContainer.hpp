#pragma once
#include <filesystem>
#include "../../../../runtime/src/graphics/debug/Imgui_Console.hpp"
#include "../features/ImguiRoot.hpp"
#include "../features/application_toolbar/Imgui_ApplicationMenuBar.hpp"
#include "../features/asset_browser/imgui/Imgui_AssetGridPanel.hpp"
#include "../features/object_inspector/imgui/Imgui_InspectorPanel.hpp"
#include "../features/scene_hierarchy/imgui/Imgui_HierarchyPanel.hpp"
#include "../features/scene_viewport/imgui/Imgui_EditorSceneViewport.hpp"
#include "../features/scene_viewport/scene_toolbar/Imgui_SceneViewToolbar.hpp"
#include "../features/scene_viewport/scene_toolbar/Imgui_SimHUD.hpp"
#include "../features/welcome/Imgui_WelcomeScreen.hpp"
#include "../services/SimulationController.hpp"

class AssetQuickActions;
class ClipboardService;
class EditorContext;
class PickingSystem;
class AssetStore;
class RuntimeScene;
class AssetThumbnailGenerator;
class UndoHistory;
class EditorCamera;
class EditorSelection;
class EditorGizmos;
class EditorSettings;
class ActionDispatcher;
class ShortcutBindingService;
class GameWindow;

class ImguiContainer {
public:
    ImguiContainer(GameWindow& window,
                   EditorCamera& editorCamera,
                   EditorSelection& editorSelection,
                   EditorGizmos& editorGizmos,
                   EditorSettings& editorSettings,
                   World& entityManager,
                   PickingSystem& pickingService,
                   RuntimeScene& scene,
                   AssetStore& assetStore,
                   AssetThumbnailGenerator& assetThumbnailGenerator,
                   EditorContext& project,
                   UndoHistory& undoHistory,
                   ActionDispatcher& actionDispatcher,
                   ShortcutBindingService& shortcutBindingService,
                   ClipboardService& clipboardService,
                   AssetQuickActions& assetQuickActions,
                   SceneQuickActions& sceneQuickActions,
                   SimulationController& simulationController,
                   Imgui_Console& console) :

        applicationMenuBar_(project, undoHistory, editorSelection, actionDispatcher, shortcutBindingService),

        sceneViewToolbar_(editorSettings, editorGizmos, simulationController),

        hierarchyPanel_(assetStore,
                        editorSelection,
                        scene,
                        actionDispatcher,
                        shortcutBindingService,
                        sceneQuickActions,
                        clipboardService),

        assetGridPanel_(assetStore, editorSelection, assetQuickActions, assetThumbnailGenerator),

        inspectorPanel_(editorSelection, assetStore, scene, editorGizmos),

        editorSceneViewport_(assetStore,
                             scene,
                             editorSelection,
                             entityManager,
                             pickingService,
                             window,
                             editorCamera,
                             actionDispatcher,
                             shortcutBindingService,
                             clipboardService,
                             sceneQuickActions),

        simHUD_(simulationController),

        imguiRoot_(applicationMenuBar_,
                   sceneViewToolbar_,
                   hierarchyPanel_,
                   assetGridPanel_,
                   inspectorPanel_,
                   editorSceneViewport_,
                   console,
                   window),

        simHUDRoot_(simHUD_, console),
        welcomeScreen_(std::filesystem::path{PROJECTS_DIR}, std::filesystem::path{SAMPLES_DIR}),
        welcomeRoot_(welcomeScreen_) {}

    ImguiRoot& imguiRoot() { return imguiRoot_; }
    SimHUDRoot& simHUDRoot() { return simHUDRoot_; }
    WelcomeRoot& welcomeRoot() { return welcomeRoot_; }
    Imgui_WelcomeScreen& welcomeScreen() { return welcomeScreen_; }
    Imgui_ApplicationMenuBar& applicationMenuBar() { return applicationMenuBar_; }

private:
    Imgui_ApplicationMenuBar applicationMenuBar_;
    Imgui_SceneViewToolbar sceneViewToolbar_;
    Imgui_HierarchyPanel hierarchyPanel_;
    Imgui_AssetGridPanel assetGridPanel_;
    Imgui_InspectorPanel inspectorPanel_;
    Imgui_EditorSceneViewport editorSceneViewport_;
    Imgui_SimHUD simHUD_;
    ImguiRoot imguiRoot_;
    SimHUDRoot simHUDRoot_;
    Imgui_WelcomeScreen welcomeScreen_;
    WelcomeRoot welcomeRoot_;
};
