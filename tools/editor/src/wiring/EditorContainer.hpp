#pragma once
#include <filesystem>
#include "../EditorApp.hpp"
#include "../features/input_handling/InputHandler.hpp"
#include "FeaturesContainer.hpp"
#include "RenderingContainer.hpp"
#include "RuntimeContainer.hpp"
#include "ServicesContainer.hpp"
#include "modules/console/commands/EchoCommand.hpp"
#include "modules/console/commands/ListActorsCommand.hpp"
#include "modules/core/GameWindow.hpp"

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
                  runtime_.simulationController()),
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
                   features_.simHUDRoot()),
        editorApp_(window,
                   runtime_.developerConsole(),
                   runtime_.inputSystem(),
                   runtime_.entityManager(),
                   features_.editorCamera(),
                   inputHandler_,
                   services_.editorSettings(),
                   services_.project(),
                   runtime_.time(),
                   runtime_.physicsSystem(),
                   rendering_.editorRenderer(),
                   runtime_.simulationController(),
                   features_.imguiConsole()) {
        runtime_.simulationController().setEditorWorld(&runtime_.entityManager());
        runtime_.simulationController().setProjectRoot(runtime_.projectRoot());
        runtime_.developerConsole().registerCommand("echo", [] { return std::make_unique<EchoCommand>(); });
        runtime_.developerConsole().registerCommand(
                "list-actors", [this] { return std::make_unique<ListActorsCommand>(runtime_.entityManager()); });
    }

    EditorApp& editorApp() { return editorApp_; }

private:
    RuntimeContainer runtime_;
    ServicesContainer services_;
    FeaturesContainer features_;
    InputHandler inputHandler_;
    RenderingContainer rendering_;
    EditorApp editorApp_;
};
