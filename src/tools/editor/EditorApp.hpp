#pragma once
#include "../../runtime/GameEngine.hpp"
#include "../../runtime/modules/core/TimeManager.hpp"
#include "../../runtime/modules/input/InputSystem.hpp"
#include "../../runtime/modules/physics/PhysicsSystem.hpp"
#include "../../runtime/modules/scene/EntityStore.hpp"
#include "features/input_handling/InputHandler.hpp"
#include "features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "rendering/EditorRenderer.hpp"
#include "services/EditorContext.hpp"
#include "services/EditorSettings.hpp"
#include "styling/ImguiStyling.hpp"

class GameWindow;
class VulkanBackend;
class EditorGizmos;
class GizmoBuilder;
class ObjectTransformHandle;
class PickingSystem;

class EditorApp {
public:
    EditorApp(GameWindow& window,
              GameEngine& engine,
              EntityManager& entityManager,
              EditorCamera& editorOrbitCamera,
              InputHandler& inputHandler,
              EditorSettings& rendererSettings,
              EditorContext& project,
              TimeManager& time,
              InputSystem& inputSystem,
              PhysicsSystem& physicsSystem,
              EditorRenderer& editorRenderer) :
        window_(window),
        engine_(engine),
        entityManager_(entityManager),
        time_(time),
        inputSystem_(inputSystem),
        physicsSystem_(physicsSystem),
        editorCamera_(editorOrbitCamera),
        editorSettings_(rendererSettings),
        project_(project),
        inputHandler_(inputHandler),
        editorRenderer_(editorRenderer) {
        initEditor();
        ImguiStyling::ApplyEditorStyle();
    }

    void run() {
        while (!window_.shouldClose()) {
            time_.beginFrame();
            float deltaTime = time_.getGameDeltaTime();

            window_.pollEvents();
            inputSystem_.update();
            physicsSystem_.update();

            auto [mouseX, mouseY] = engine_.getMousePosition();
            inputHandler_.update(mouseX, mouseY, deltaTime);

            float gridScale = editorSettings_.getGridScale();
            editorRenderer_.render(entityManager_, gridScale);

            time_.endFrame();
        }
    }

private:
    static SceneViewport computeSceneViewport(int w, int h) {
        constexpr float HIERARCHY_WIDTH_RATIO = 0.15f;
        constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
        constexpr float ASSETS_HEIGHT_RATIO = 0.3f;
        int left = static_cast<int>(static_cast<float>(w) * HIERARCHY_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - INSPECTOR_WIDTH_RATIO));
        int bottom = static_cast<int>(static_cast<float>(h) * (1.0f - ASSETS_HEIGHT_RATIO));
        return {left, 0, right - left, bottom};
    }

    void initEditor() {
        setupViewport();
        editorSettings_.enableGrid();
        SceneViewport sv = window_.getSceneViewport();
        if (sv.width > 0 && sv.height > 0)
            editorCamera_.setAspectRatio(static_cast<float>(sv.width) / static_cast<float>(sv.height));

        if (!project_.loadLatestScene()) project_.newScene();
    }

    void setupViewport() {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport(computeSceneViewport(w, h));
        window_.onWindowResize([this](int newW, int newH, int, int) {
            SceneViewport sv = computeSceneViewport(newW, newH);
            window_.setSceneViewport(sv);
            if (sv.width > 0 && sv.height > 0)
                editorCamera_.setAspectRatio(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        });
    }

    GameWindow& window_;
    GameEngine& engine_;
    EntityManager& entityManager_;
    TimeManager& time_;
    InputSystem& inputSystem_;
    PhysicsSystem& physicsSystem_;
    EditorCamera& editorCamera_;
    EditorSettings& editorSettings_;
    EditorContext& project_;
    InputHandler& inputHandler_;
    EditorRenderer& editorRenderer_;
};
