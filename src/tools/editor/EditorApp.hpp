#pragma once
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <string>
#include <unordered_set>
#include "../../engine/GameEngine.hpp"
#include "../../engine/data/RenderTargetHandle.hpp"
#include "../../engine/data/components/Transform.hpp"
#include "../../engine/systems/assets/AssetManager.hpp"
#include "../../engine/systems/ui/UserInterface.hpp"
#include "controller/EditorPickingSystem.hpp"
#include "controller/EditorUIController.hpp"
#include "controller/OrbitCameraController.hpp"
#include "controller/RenderingController.hpp"
#include "state/EditorState.hpp"
#include "view/imgui_widgets/containers/HierarchyPanel.hpp"
#include "view/imgui_widgets/containers/InspectorPanel.hpp"
#include "view/imgui_widgets/containers/RenderTargetPreview.hpp"

class GameWindow;
class EditorRenderSystem;
class VulkanEditorRenderer;
class RendererSettings;

class EditorApp {
public:
    EditorApp(GameWindow& window,
              GameEngine& engine,
              EditorState& editorState,
              EditorRenderSystem& editorRenderSystem,
              VulkanEditorRenderer& editorRenderer,
              RendererSettings& rendererSettings,
              AssetManager& assetManager,
              UserInterface& userInterface) :
        window_(window),
        engine_(engine),
        editorState_(editorState),
        assetManager_(assetManager),
        userInterface_(userInterface),
        renderingController_(editorState_, editorRenderer, editorRenderSystem, rendererSettings, assetManager),
        uiController_(engine_, userInterface, editorState_),
        cameraController_(engine_),
        pickingSystem_(assetManager) {
        setupViewport();
        setupScene();
        setupRenderTargetPreview();
    }

    // Called each frame by the wiring container's main loop
    void update(double deltaTime) {
        handleInput(deltaTime);
        updateCamera(deltaTime);
        updateRenderTarget();
    }

private:
    GameWindow& window_;
    GameEngine& engine_;
    EditorState& editorState_;
    AssetManager& assetManager_;
    UserInterface& userInterface_;
    RenderingController renderingController_;
    EditorUIController uiController_;
    OrbitCameraController cameraController_;
    EditorPickingSystem pickingSystem_;

    bool wasLeftDown_ = false;
    RenderTargetHandle renderTarget_;
    Camera previewCamera_;
    Transform previewCameraTransform_;

    // Gizmo visualization state
    std::unordered_set<std::string> aabbEnabled_;
    bool bvhEnabled_ = false;

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

    void setupRenderTargetPreview() {
        renderTarget_ = renderingController_.createRenderTarget(512, 512);
        previewCamera_.aspect = 1.0f;
        previewCamera_.fov = 60.0f;
        previewCameraTransform_.position = {0.0f, 15.0f, 0.0f};
        previewCameraTransform_.scale = {1.0f, 1.0f, 1.0f};
        previewCameraTransform_.rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1, 0, 0));
        userInterface_.emplace<RenderTargetPreview>(engine_, renderingController_, renderTarget_);
    }

    void handleInput(double deltaTime) {
        auto [mouseX, mouseY] = engine_.getMousePosition();

        bool ctrlHeld = engine_.isKeyDown(GLFW_KEY_LEFT_CONTROL) || engine_.isKeyDown(GLFW_KEY_RIGHT_CONTROL);
        bool shiftHeld = engine_.isKeyDown(GLFW_KEY_LEFT_SHIFT) || engine_.isKeyDown(GLFW_KEY_RIGHT_SHIFT);

        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_S) && uiController_.scenePath)
            uiController_.saveScene(*uiController_.scenePath);
        if (ctrlHeld && engine_.isKeyPressed(GLFW_KEY_Z)) {
            if (shiftHeld)
                uiController_.mutations.undoHistory().redo();
            else
                uiController_.mutations.undoHistory().undo();
        }

        if (engine_.isKeyPressed(GLFW_KEY_G)) renderingController_.toggleWorldGrid();
        if (engine_.isKeyPressed(GLFW_KEY_L)) renderingController_.toggleLighting();
        if (engine_.isKeyPressed(GLFW_KEY_B)) bvhEnabled_ = !bvhEnabled_;

        bool leftDown = engine_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
        if (leftDown && !wasLeftDown_ && !ImGui::GetIO().WantCaptureMouse) {
            auto sv = window_.getSceneViewport();
            auto result = pickingSystem_.pick(static_cast<uint32_t>(mouseX),
                                              static_cast<uint32_t>(mouseY),
                                              static_cast<uint32_t>(sv.x),
                                              static_cast<uint32_t>(sv.y),
                                              static_cast<uint32_t>(sv.width),
                                              static_cast<uint32_t>(sv.height),
                                              cameraController_.getCamera(),
                                              cameraController_.getTransform(),
                                              editorState_.getScene());
            if (result) {
                std::visit([this](const auto& hit) { onPickResult(hit); }, *result);
            } else {
                editorState_.setSelectedObject({});
            }
        }
        wasLeftDown_ = leftDown;

        drawGizmos();
    }

    void onPickResult(const SceneObjectHit& hit) {
        auto current = editorState_.getSelectedObject();
        if (current == hit.objectId)
            editorState_.setSelectedObject({});
        else
            editorState_.setSelectedObject(hit.objectId);
    }

    void onPickResult(const GizmoHit& /*hit*/) {
        // TODO: placeholder — gizmo handle drag logic (translation/rotation/scale) goes here
    }

    void drawGizmos() {
        pickingSystem_.clearHandles(); // TODO: placeholder — will re-register gizmo handles each frame once handles are
                                       // implemented

        // Draw outline for selected object
        if (auto selectedId = editorState_.getSelectedObject()) {
            renderingController_.drawObjectOutline(*selectedId);
        }

        // Draw transform axes for selected object
        if (auto selectedId = editorState_.getSelectedObject()) {
            renderingController_.drawGizmoObjectTransform(*selectedId, 1.0f);
        }

        // Draw AABBs for enabled objects
        for (const auto& objectId: aabbEnabled_) {
            renderingController_.drawGizmoObjectAABB(objectId, {0.0f, 1.0f, 0.0f});
        }

        // Draw BVH visualization if enabled
        if (bvhEnabled_) {
            renderingController_.drawGizmoBVH({1.0f, 1.0f, 0.0f});
        }
    }

    // Gizmo control methods
    void enableObjectAABB(const std::string& objectId) { aabbEnabled_.insert(objectId); }
    void disableObjectAABB(const std::string& objectId) { aabbEnabled_.erase(objectId); }
    void toggleBVH() { bvhEnabled_ = !bvhEnabled_; }

    void updateCamera(double deltaTime) {
        cameraController_.update(deltaTime);
        renderingController_.setActiveCamera(cameraController_.getCamera(), cameraController_.getTransform());
    }

    void updateRenderTarget() {
        if (renderTarget_.isValid())
            renderingController_.renderToTarget(renderTarget_, previewCamera_, previewCameraTransform_);
    }
};
