#pragma once
#include <string>
#include "../../engine/GameEngine.hpp"
#include "../../engine/modules/core/TimeManager.hpp"
#include "../../engine/modules/input/InputSystem.hpp"
#include "../../engine/modules/physics/PhysicsSystem.hpp"
#include "../../engine/modules/scene/EntityManager.hpp"
#include "business/EditorOrbitCamera.hpp"
#include "business/EditorSettings.hpp"
#include "business/SceneLoader.hpp"
#include "business/scene_editing/ObjectPrefabs.hpp"
#include "input/InputHandler.hpp"
#include "presentation/PresentationLayer.hpp"
// TODO: Refactor UI panels to use EntityManager
// #include "presentation/imgui/containers/HierarchyPanel.hpp"
// #include "presentation/imgui/containers/InspectorPanel.hpp"

class HierarchyPanel;
class InspectorPanel;

class GameWindow;

class EditorApp {
public:
    EditorApp(GameWindow& window,
              GameEngine& engine,
              EntityManager& entityManager,
              EditorOrbitCamera& editorOrbitCamera,
              InputHandler& inputHandler,
              PresentationLayer& presentationLayer,
              EditorSettings& rendererSettings,
              SceneLoader& sceneLoader,
              TimeManager& time,
              InputSystem& inputSystem,
              PhysicsSystem& physicsSystem) :
        window_(window),
        engine_(engine),
        entityManager_(entityManager),
        time_(time),
        inputSystem_(inputSystem),
        physicsSystem_(physicsSystem),
        editorOrbitCamera_(editorOrbitCamera),
        rendererSettings_(rendererSettings),
        presentationLayer_(presentationLayer),
        sceneLoader_(sceneLoader),
        inputHandler_(inputHandler) {
        initEditor();
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

            float gridScale = rendererSettings_.getGridScale();
            presentationLayer_.render(entityManager_, gridScale, {});
            time_.endFrame();
        }
    }

private:
    static SceneViewport computeSceneViewport(int w, int h) {
        constexpr float HIERARCHY_WIDTH_RATIO = 0.15f;
        constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
        int left = static_cast<int>(static_cast<float>(w) * HIERARCHY_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - INSPECTOR_WIDTH_RATIO));
        return {left, 0, right - left, h};
    }

    void initEditor() {
        setupViewport();
        rendererSettings_.enableGrid();
        SceneViewport sv = window_.getSceneViewport();
        if (sv.width > 0 && sv.height > 0)
            editorOrbitCamera_.setAspectRatio(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        sceneLoader_.newScene();
        entityManager_.upsertFromJson(primitives::cube());
    }

    void setupViewport() {
        auto [w, h] = window_.getLogicalSize();
        window_.setSceneViewport(computeSceneViewport(w, h));
        window_.onWindowResize([this](int newW, int newH, int, int) {
            SceneViewport sv = computeSceneViewport(newW, newH);
            window_.setSceneViewport(sv);
            if (sv.width > 0 && sv.height > 0)
                editorOrbitCamera_.setAspectRatio(static_cast<float>(sv.width) / static_cast<float>(sv.height));
        });
    }

    GameWindow& window_;
    GameEngine& engine_;
    EntityManager& entityManager_;
    TimeManager& time_;
    InputSystem& inputSystem_;
    PhysicsSystem& physicsSystem_;
    EditorOrbitCamera& editorOrbitCamera_;
    EditorSettings& rendererSettings_;
    PresentationLayer& presentationLayer_;
    SceneLoader& sceneLoader_;
    InputHandler& inputHandler_;
};
