#pragma once
#include <filesystem>
#include "../EditorApp.hpp"
#include "../features/input_handling/InputHandler.hpp"
#include "../features/terrain_tool/Imgui_TerrainPanel.hpp"
#include "../features/terrain_tool/TerrainPaintTool.hpp"
#include "../features/terrain_tool/TerrainSculptTool.hpp"
#include "../services/MaterialPreviewRenderer.hpp"
#include "FeaturesContainer.hpp"
#include "RenderingContainer.hpp"
#include "RuntimeContainer.hpp"
#include "ServicesContainer.hpp"
#include "console/commands/EchoCommand.hpp"
#include "console/commands/ListActorsCommand.hpp"
#include "core/GameWindow.hpp"
#include "core/assets/PackagePaths.hpp"

class EditorContainer {
public:
    explicit EditorContainer(GameWindow& window,
                             const std::vector<std::filesystem::path>& mountPaths,
                             const std::filesystem::path& projectRoot) :
        runtime_(window, mountPaths, projectRoot),
        services_(runtime_.assetLoader(), runtime_.rendererSettings(), runtime_.vfs(), runtime_.entityManager()),
        features_(window,
                  runtime_.assetLoader(),
                  runtime_.entityManager(),
                  runtime_.inputSystem(),
                  runtime_.developerConsole(),
                  runtime_.assetThumbnailGenerator(),
                  services_.scene(),
                  services_.assetStore(),
                  services_.project(),
                  services_.undoHistory(),
                  services_.editorSelection(),
                  services_.editorSettings(),
                  services_.assetQuickActions(),
                  services_.sceneQuickActions(),
                  services_.clipboardService(),
                  runtime_.simulationController(),
                  services_.editorModeService()),
        inputHandler_(window,
                      runtime_.inputSystem(),
                      features_.pickingSystem(),
                      features_.editorCamera(),
                      services_.editorSelection(),
                      features_.objectTransformHandle(),
                      runtime_.entityManager(),
                      services_.scene(),
                      features_.editorGizmos(),
                      services_.editorSettings(),
                      services_.undoHistory(),
                      services_.clipboardService(),
                      features_.shortcutBindingService(),
                      features_.actionDispatcher()),
        rendering_(window,
                   runtime_,
                   features_.editorCamera(),
                   services_.editorSelection(),
                   features_.editorGizmos(),
                   runtime_.entityManager(),
                   features_.pickingSystem(),
                   features_.gizmosBuilder(),
                   features_.objectTransformHandle(),
                   features_.imguiRoot(),
                   features_.simHUDRoot(),
                   features_.welcomeRoot(),
                   services_.editorModeService()),
        terrainSculptTool_(runtime_.entityManager(),
                           services_.scene(),
                           services_.editorSelection(),
                           services_.assetStore(),
                           features_.editorCamera(),
                           runtime_.inputSystem(),
                           window,
                           runtime_.vulkanBackend(),
                           services_.undoHistory()),
        terrainPaintTool_(runtime_.entityManager(),
                          services_.scene(),
                          services_.editorSelection(),
                          services_.assetStore(),
                          features_.editorCamera(),
                          runtime_.inputSystem(),
                          window,
                          runtime_.vulkanBackend(),
                          services_.undoHistory()),
        terrainPanel_(terrainSculptTool_,
                      terrainPaintTool_,
                      services_.sceneQuickActions(),
                      services_.editorSelection(),
                      features_.assetPicker()),
        editorApp_(window,
                   runtime_.developerConsole(),
                   runtime_.inputSystem(),
                   runtime_.entityManager(),
                   features_.editorCamera(),
                   inputHandler_,
                   services_.editorSettings(),
                   services_.project(),
                   services_.undoHistory(),
                   runtime_.time(),
                   runtime_.physicsSystem(),
                   rendering_.editorRenderer(),
                   features_.imguiRoot(),
                   runtime_.simulationController(),
                   features_.imguiConsole(),
                   features_.welcomeScreen(),
                   runtime_.animationSystem(),
                   runtime_.assetThumbnailGenerator(),
                   terrainSculptTool_,
                   terrainPaintTool_,
                   services_.editorModeService()),
        materialPreviewRenderer_(runtime_.vulkanBackend(), runtime_.assetLoader(), runtime_.loadedAssets()) {
        runtime_.assetThumbnailGenerator().setMaterialPreviewRenderer(&materialPreviewRenderer_);
        services_.assetStore().setOnAssetMutated([this](const std::string& assetName) {
            if (assetName.ends_with(".mat") || assetName.ends_with(".mesh") || assetName.ends_with(".model") ||
                assetName.ends_with(".matpkg") || assetName.ends_with(".modelpkg") ||
                assetName.ends_with(".terrainpkg"))
                runtime_.assetThumbnailGenerator().invalidate(assetName);
            if (auto pkg = PackagePaths::enclosingPackage(assetName))
                runtime_.assetThumbnailGenerator().invalidate(*pkg);
        });
        features_.imguiRoot().setTerrainPanel(&terrainPanel_);
        runtime_.simulationController().setEditorWorld(&runtime_.entityManager());
        runtime_.simulationController().setProjectRoot(runtime_.projectRoot());
        if (!runtime_.projectRoot().empty()) {
            services_.project().initProject();
        }
        runtime_.developerConsole().registerCommand("echo", [] { return std::make_unique<EchoCommand>(); });
        runtime_.developerConsole().registerCommand(
                "list-actors", [this] { return std::make_unique<ListActorsCommand>(runtime_.entityManager()); });
        features_.applicationMenuBar().setProjectCallbacks({
                [this] { editorApp_.newProject(); },
                [this](const std::filesystem::path& path) { editorApp_.openProject(path); },
        });
    }

    EditorApp& editorApp() { return editorApp_; }

private:
    RuntimeContainer runtime_;
    ServicesContainer services_;
    FeaturesContainer features_;
    InputHandler inputHandler_;
    RenderingContainer rendering_;
    TerrainSculptTool terrainSculptTool_;
    TerrainPaintTool terrainPaintTool_;
    Imgui_TerrainPanel terrainPanel_;
    EditorApp editorApp_;
    MaterialPreviewRenderer materialPreviewRenderer_;
};
