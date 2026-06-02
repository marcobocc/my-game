#pragma once
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include "../../../runtime/src/modules/console/ConsoleLogAppender.hpp"
#include "features/input_handling/InputHandler.hpp"
#include "features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/core/TimeManager.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/physics/PhysicsSystem.hpp"
#include "modules/scene/World.hpp"
#include "rendering/EditorRenderer.hpp"
#include "services/EditorContext.hpp"
#include "services/EditorSettings.hpp"
#include "services/SimulationController.hpp"
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
              DeveloperConsole& developerConsole,
              InputSystem& inputSystem,
              World& entityManager,
              EditorCamera& editorOrbitCamera,
              InputHandler& inputHandler,
              EditorSettings& rendererSettings,
              EditorContext& project,
              TimeManager& time,
              PhysicsSystem& physicsSystem,
              EditorRenderer& editorRenderer,
              SimulationController& simulationController) :
        window_(window),
        developerConsole_(developerConsole),
        inputSystem_(inputSystem),
        entityManager_(entityManager),
        time_(time),
        physicsSystem_(physicsSystem),
        editorCamera_(editorOrbitCamera),
        editorSettings_(rendererSettings),
        project_(project),
        inputHandler_(inputHandler),
        editorRenderer_(editorRenderer),
        simulationController_(simulationController) {
        initEditor();
        ImguiStyling::ApplyEditorStyle();
    }

    void run() {
        setupConsoleAppender();
        while (!window_.shouldClose()) {
            window_.pollEvents();
            time_.beginFrame();
            float deltaTime = time_.getGameDeltaTime();

            simulationController_.flushPendingStop();

            bool simActive = simulationController_.isActive();
            if (simActive != simWasActive_) {
                editorRenderer_.setSimMode(simActive);
                simWasActive_ = simActive;
            }

            inputSystem_.update();
            auto [mouseX, mouseY] = inputSystem_.getMousePosition();
            inputHandler_.update(mouseX, mouseY, deltaTime);
            developerConsole_.tick();

            if (simActive) {
                simulationController_.tick(deltaTime);
                editorRenderer_.render(simulationController_.world(), 0.0f);
            } else {
                editorRenderer_.render(entityManager_, editorSettings_.getGridScale());
            }

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

    void setupConsoleAppender() {
        auto appender = std::make_shared<ConsoleLogAppender>(&developerConsole_);
        appender->setLayout(std::make_shared<log4cxx::PatternLayout>("%d %-5p %c - %m%n"));
        log4cxx::Logger::getRootLogger()->addAppender(appender);
    }

    GameWindow& window_;
    DeveloperConsole& developerConsole_;
    InputSystem& inputSystem_;
    World& entityManager_;
    TimeManager& time_;
    PhysicsSystem& physicsSystem_;
    EditorCamera& editorCamera_;
    EditorSettings& editorSettings_;
    EditorContext& project_;
    InputHandler& inputHandler_;
    EditorRenderer& editorRenderer_;
    SimulationController& simulationController_;
    bool simWasActive_ = false;
};
