#pragma once
#include <filesystem>
#include "../../../runtime/modules/core/GameWindow.hpp"
#include "../EditorApp.hpp"
#include "../features/input_handling/InputHandler.hpp"
#include "FeaturesContainer.hpp"
#include "RenderingContainer.hpp"
#include "RuntimeContainer.hpp"
#include "ServicesContainer.hpp"

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
                  runtime_.engine(),
                  runtime_.assetThumbnailGenerator(),
                  services_.scene(),
                  services_.assetStore(),
                  services_.project(),
                  services_.undoHistory(),
                  services_.editorSelection(),
                  services_.editorSettings(),
                  services_.assetQuickActions(),
                  services_.sceneQuickActions(),
                  services_.clipboardService()),
        inputHandler_(window,
                      runtime_.engine(),
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
                   features_.imguiRoot()),
        editorApp_(window,
                   runtime_.engine(),
                   runtime_.entityManager(),
                   features_.editorCamera(),
                   inputHandler_,
                   services_.editorSettings(),
                   services_.project(),
                   runtime_.time(),
                   runtime_.inputSystem(),
                   runtime_.physicsSystem(),
                   rendering_.editorRenderer()) {}

    EditorApp& editorApp() { return editorApp_; }

private:
    RuntimeContainer runtime_;
    ServicesContainer services_;
    FeaturesContainer features_;
    InputHandler inputHandler_;
    RenderingContainer rendering_;
    EditorApp editorApp_;
};
