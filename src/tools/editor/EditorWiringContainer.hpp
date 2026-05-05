#pragma once
#include <filesystem>
#include "../../engine/GameEngineWiringContainer.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanGizmoPass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanGridPass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanObjectIdPass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanOutlinePass.hpp"
#include "../../engine/systems/rendering/vulkan/passes/VulkanUIPass.hpp"
#include "EditorApp.hpp"
#include "controller/EditorPickingSystem.hpp"
#include "controller/EditorUIController.hpp"
#include "controller/GizmoController.hpp"
#include "controller/InteractionController.hpp"
#include "controller/OrbitCameraController.hpp"
#include "controller/RenderingController.hpp"
#include "controller/ShortcutController.hpp"
#include "controller/TransformController.hpp"
#include "rendering/EditorRenderSystem.hpp"
#include "rendering/VulkanEditorRenderer.hpp"
#include "state/EditorState.hpp"

/*
    EditorWiringContainer

    Purpose:
    --------------------------------------------------
    Central composition root for the editor application.

    Responsibilities:
    --------------------------------------------------
    - Extend GameEngineWiringContainer with editor-specific logic
    - Create editor-only render passes and the editor renderer
    - Create and wire EditorState, controllers, and EditorApp
    - Manage the editor application lifecycle

    This class is not responsible for:
    --------------------------------------------------
    - Core engine setup (delegated to GameEngineWiringContainer)
    - Application logic (delegated to EditorApp)
*/
class EditorWiringContainer : public GameEngineWiringContainer {
public:
    EditorWiringContainer(GameWindow& window, const std::filesystem::path& assetsPath) :
        GameEngineWiringContainer(window, assetsPath),
        gridPass_(assetManager_, resourcesManager_),
        gizmoPass_(vulkanContext_, assetManager_, resourcesManager_, swapchainManager_),
        objectIdPass_(vulkanContext_, assetManager_, resourcesManager_),
        outlinePass_(
                vulkanContext_, assetManager_, resourcesManager_, swapchainManager_.swapchain().swapchainImageFormat),
        uiPass_(vulkanContext_, swapchainManager_, window, userInterface_),
        editorRenderer_(window,
                        vulkanContext_,
                        frameManager_,
                        renderTargetManager_,
                        geometryPass_,
                        lightingPass_,
                        gridPass_,
                        gizmoPass_,
                        objectIdPass_,
                        outlinePass_,
                        uiPass_,
                        swapchainManager_,
                        rendererSettings_),
        editorRenderSystem_(editorRenderer_),
        editorState_(scene_),
        gizmoController_(editorState_, assetManager_),
        renderingController_(editorState_, editorRenderer_, editorRenderSystem_, rendererSettings_, assetManager_),
        orbitalCamera_(engine_),
        pickingSystem_(assetManager_),
        uiController_(engine_, userInterface_, editorState_),
        transformController_(editorState_, window, engine_, uiController_, orbitalCamera_),
        shortcutController_(engine_, uiController_, renderingController_, editorState_, transformController_),
        interactionController_(window, engine_, editorState_, pickingSystem_, transformController_),
        editorApp_(window,
                   engine_,
                   editorState_,
                   assetManager_,
                   userInterface_,
                   renderingController_,
                   gizmoController_,
                   uiController_,
                   orbitalCamera_,
                   pickingSystem_,
                   transformController_,
                   shortcutController_,
                   interactionController_) {}

    EditorApp& editorApp() { return editorApp_; }

    void run() {
        while (!window_.shouldClose()) {
            time_.beginFrame();
            float deltaTime = time_.getGameDeltaTime();

            editorState_.clearGizmoLines();
            editorState_.clearOutlineQueue();

            window_.pollEvents();
            inputSystem_.update();
            physicsSystem_.update();
            editorApp_.update(deltaTime);

            editorRenderSystem_.update(
                    editorState_.getScene(), editorState_.getOutlineQueue(), editorState_.getGizmoLines());
            time_.endFrame();
        }
    }

private:
    VulkanGridPass gridPass_;
    VulkanGizmoPass gizmoPass_;
    VulkanObjectIdPass objectIdPass_;
    VulkanOutlinePass outlinePass_;
    VulkanUIPass uiPass_;
    VulkanEditorRenderer editorRenderer_;
    EditorRenderSystem editorRenderSystem_;
    EditorState editorState_;
    GizmoController gizmoController_;
    RenderingController renderingController_;
    OrbitCameraController orbitalCamera_;
    EditorPickingSystem pickingSystem_;
    EditorUIController uiController_;
    TransformController transformController_;
    ShortcutController shortcutController_;
    InteractionController interactionController_;
    EditorApp editorApp_;
};
