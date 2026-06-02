#pragma once
#include "../../../../runtime/src/modules/console/Imgui_Console.hpp"
#include "../features/input_handling/ShortcutBindingService.hpp"
#include "../features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "../features/scene_viewport/gizmos/EditorGizmos.hpp"
#include "../features/scene_viewport/gizmos/GizmoBuilder.hpp"
#include "../features/scene_viewport/picking/PickingSystem.hpp"
#include "../features/scene_viewport/transform_handle/ObjectTransformHandle.hpp"
#include "../services/ActionDispatcher.hpp"
#include "ImguiContainer.hpp"
#include "RuntimeContainer.hpp"
#include "ServicesContainer.hpp"

class FeaturesContainer {
public:
    explicit FeaturesContainer(GameWindow& window,
                               AssetLoader& assetLoader,
                               World& entityManager,
                               GameEngine& engine,
                               AssetThumbnailGenerator& assetThumbnailGenerator,
                               RuntimeScene& scene,
                               AssetStore& assetStore,
                               EditorContext& project,
                               UndoHistory& undoHistory,
                               EditorSelection& editorSelection,
                               EditorSettings& editorSettings,
                               AssetQuickActions& assetQuickActions,
                               SceneQuickActions& sceneQuickActions,
                               ClipboardService& clipboardService) :
        imguiConsole_(engine.developerConsole()),
        gizmosBuilder_(assetLoader, entityManager, editorSettings),
        pickingSystem_(assetLoader),
        objectTransformHandle_(window, engine, scene, editorCamera_, editorSelection, editorSettings),
        actionDispatcher_(undoHistory,
                          editorCamera_,
                          editorGizmos_,
                          editorSettings,
                          objectTransformHandle_,
                          sceneQuickActions,
                          project,
                          imguiConsole_),
        imgui_(window,
               editorCamera_,
               editorSelection,
               editorGizmos_,
               editorSettings,
               entityManager,
               pickingSystem_,
               scene,
               assetStore,
               assetThumbnailGenerator,
               project,
               undoHistory,
               actionDispatcher_,
               shortcutBindingService_,
               clipboardService,
               assetQuickActions,
               sceneQuickActions,
               engine,
               imguiConsole_) {}

    EditorGizmos& editorGizmos() { return editorGizmos_; }
    EditorCamera& editorCamera() { return editorCamera_; }
    GizmoBuilder& gizmosBuilder() { return gizmosBuilder_; }
    PickingSystem& pickingSystem() { return pickingSystem_; }
    ObjectTransformHandle& objectTransformHandle() { return objectTransformHandle_; }
    ShortcutBindingService& shortcutBindingService() { return shortcutBindingService_; }
    ActionDispatcher& actionDispatcher() { return actionDispatcher_; }
    ImguiRoot& imguiRoot() { return imgui_.imguiRoot(); }

private:
    Imgui_Console imguiConsole_;
    EditorGizmos editorGizmos_;
    EditorCamera editorCamera_;
    GizmoBuilder gizmosBuilder_;
    PickingSystem pickingSystem_;
    ObjectTransformHandle objectTransformHandle_;
    ShortcutBindingService shortcutBindingService_;
    ActionDispatcher actionDispatcher_;
    ImguiContainer imgui_;
};
