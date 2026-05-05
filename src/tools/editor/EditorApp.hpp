#pragma once
#include <optional>
#include <string>
#include <unordered_set>
#include "../../engine/GameEngine.hpp"
#include "../../engine/systems/assets/AssetManager.hpp"
#include "../../engine/systems/input/RaycastPickingSystem.hpp"
#include "../../engine/systems/ui/UserInterface.hpp"
#include "controller/EditorPickingSystem.hpp"
#include "controller/EditorUIController.hpp"
#include "controller/GizmoController.hpp"
#include "controller/InteractionController.hpp"
#include "controller/OrbitCameraController.hpp"
#include "controller/RenderingController.hpp"
#include "controller/ShortcutController.hpp"
#include "controller/TransformController.hpp"
#include "state/EditorState.hpp"
#include "state/TransformDragState.hpp"
#include "view/imgui_widgets/containers/HierarchyPanel.hpp"
#include "view/imgui_widgets/containers/InspectorPanel.hpp"

class GameWindow;
class EditorRenderSystem;
class VulkanEditorRenderer;
struct RendererSettings;

class EditorApp {
public:
    EditorApp(GameWindow& window,
              GameEngine& engine,
              EditorState& editorState,
              AssetManager& assetManager,
              UserInterface& userInterface,
              RenderingController& renderingController,
              GizmoController& gizmoController,
              EditorUIController& uiController,
              OrbitCameraController& cameraController,
              EditorPickingSystem& pickingSystem,
              TransformController& transformController,
              ShortcutController& shortcutController,
              InteractionController& interactionController) :
        window_(window),
        engine_(engine),
        editorState_(editorState),
        assetManager_(assetManager),
        userInterface_(userInterface),
        renderingController_(renderingController),
        gizmoController_(gizmoController),
        uiController_(uiController),
        cameraController_(cameraController),
        pickingSystem_(pickingSystem),
        transformController_(transformController),
        shortcutController_(shortcutController),
        interactionController_(interactionController) {
        setupViewport();
        setupScene();
    }

    void update(double deltaTime) {
        handleInput(deltaTime);
        updateCamera(deltaTime);
    }

private:
    GameWindow& window_;
    GameEngine& engine_;
    EditorState& editorState_;
    AssetManager& assetManager_;
    UserInterface& userInterface_;
    RenderingController& renderingController_;
    GizmoController& gizmoController_;
    EditorUIController& uiController_;
    OrbitCameraController& cameraController_;
    EditorPickingSystem& pickingSystem_;
    TransformController& transformController_;
    ShortcutController& shortcutController_;
    InteractionController& interactionController_;

    static SceneViewport computeSceneViewport(int w, int h) {
        int left = static_cast<int>(static_cast<float>(w) * HierarchyPanel::PANEL_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - InspectorPanel::PANEL_WIDTH_RATIO));
        return {left, 0, right - left, h};
    }

    void setupViewport() {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport(computeSceneViewport(w, h));
        window_.onWindowResize([this](int newW, int newH, int, int) {
            SceneViewport sv = computeSceneViewport(newW, newH);
            window_.setSceneViewport(sv);
            if (sv.width > 0 && sv.height > 0)
                cameraController_.setAspect(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        });
    }

    void setupScene() {
        SceneViewport sv = window_.getSceneViewport();
        if (sv.width > 0 && sv.height > 0)
            cameraController_.setAspect(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        engine_.createCube({});
        renderingController_.enableWorldGrid();
    }

    void handleInput(double deltaTime) {
        auto [mouseX, mouseY] = engine_.getMousePosition();
        shortcutController_.update();
        interactionController_.update(mouseX, mouseY, cameraController_.getCamera(), cameraController_.getTransform());
        drawGizmos();
    }

    void drawGizmos() {
        pickingSystem_.clearHandles();
        if (auto selectedId = editorState_.getSelectedObject()) {
            renderingController_.drawObjectOutline(*selectedId);
            std::vector<GizmoHandle> handles;
            auto& dragState = transformController_.getDragState();
            if (dragState.gizmoMode == GizmoType::Translation) {
                handles = gizmoController_.drawTranslationHandles(
                        *selectedId, cameraController_.getCamera(), cameraController_.getTransform());
            } else if (dragState.gizmoMode == GizmoType::Rotation) {
                handles = gizmoController_.drawRotationHandles(
                        *selectedId, cameraController_.getCamera(), cameraController_.getTransform());
            } else if (dragState.gizmoMode == GizmoType::Scale) {
                handles = gizmoController_.drawScaleHandles(
                        *selectedId, cameraController_.getCamera(), cameraController_.getTransform());
            }
            for (const auto& h: handles)
                pickingSystem_.registerHandle(h);
        }
        for (const auto& objectId: editorState_.getAABBEnabledObjects())
            gizmoController_.drawGizmoObjectAABB(objectId, {0.0f, 1.0f, 0.0f});

        for (const auto& objectId: editorState_.getBoundingSphereEnabledObjects())
            gizmoController_.drawGizmoObjectBoundingSphere(objectId, {1.0f, 1.0f, 0.0f});

        if (editorState_.isBVHEnabled()) gizmoController_.drawGizmoBVH({1.0f, 1.0f, 0.0f});
    }

    void updateCamera(double deltaTime) {
        cameraController_.update(deltaTime);
        renderingController_.setActiveCamera(cameraController_.getCamera(), cameraController_.getTransform());
    }
};
