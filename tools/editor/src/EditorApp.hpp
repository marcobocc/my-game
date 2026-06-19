#pragma once
#include <filesystem>
#include <imgui.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include <tracy/Tracy.hpp>
#include "../../../runtime/src/modules/console/ConsoleLogAppender.hpp"
#include "../../../runtime/src/modules/console/Imgui_Console.hpp"
#include "features/ImguiRoot.hpp"
#include "features/close_modal/Imgui_CloseModal.hpp"
#include "features/input_handling/InputHandler.hpp"
#include "features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "features/welcome/Imgui_WelcomeScreen.hpp"
#include "modules/console/DeveloperConsole.hpp"
#include "modules/core/GameWindow.hpp"
#include "modules/core/TimeManager.hpp"
#include "modules/input/InputSystem.hpp"
#include "modules/physics/PhysicsSystem.hpp"
#include "modules/scene/World.hpp"
#include "rendering/EditorRenderer.hpp"
#include "services/EditorContext.hpp"
#include "services/EditorSettings.hpp"
#include "services/SimulationController.hpp"
#include "services/UndoHistory.hpp"
#include "styling/ImguiStyling.hpp"

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
              UndoHistory& undoHistory,
              TimeManager& time,
              PhysicsSystem& physicsSystem,
              EditorRenderer& editorRenderer,
              ImguiRoot& imguiRoot,
              SimulationController& simulationController,
              Imgui_Console& imguiConsole,
              Imgui_WelcomeScreen& welcomeScreen) :
        window_(window),
        developerConsole_(developerConsole),
        inputSystem_(inputSystem),
        entityManager_(entityManager),
        time_(time),
        physicsSystem_(physicsSystem),
        editorCamera_(editorOrbitCamera),
        editorSettings_(rendererSettings),
        project_(project),
        undoHistory_(undoHistory),
        inputHandler_(inputHandler),
        editorRenderer_(editorRenderer),
        imguiRoot_(imguiRoot),
        simulationController_(simulationController),
        imguiConsole_(imguiConsole),
        welcomeScreen_(welcomeScreen) {
        welcomeScreen_.setCallbacks({
                [this](const std::filesystem::path& path) { openProject(path); },
                [this] { newProject(); },
        });
        initApp();
        ImguiStyling::ApplyEditorStyle();
    }

    void openProject(const std::filesystem::path& projectRoot) {
        project_.openProject(projectRoot);
        simulationController_.setProjectRoot(projectRoot);
        enterEditor();
    }

    void newProject() {
        std::filesystem::path projectsRoot{PROJECTS_DIR};
        std::filesystem::path newPath = projectsRoot / "NewProject";
        int index = 2;
        while (std::filesystem::exists(newPath)) {
            newPath = projectsRoot / ("NewProject" + std::to_string(index));
            ++index;
        }
        std::filesystem::create_directories(newPath);
        openProject(newPath);
    }

    void run() {
        setupConsoleAppender();
        while (!window_.shouldClose()) {
            ZoneScoped;
            window_.pollEvents();
            time_.beginFrame();
            float deltaTime = time_.getGameDeltaTime();

            if (appState_ == AppState::Welcome) {
                editorRenderer_.renderWelcome();
                time_.endFrame();
                FrameMark;
                continue;
            }

            simulationController_.flushPendingStop();

            bool simActive = simulationController_.isActive();
            if (simActive != simWasActive_) {
                editorRenderer_.setSimMode(simActive);
                if (simActive) {
                    auto [w, h] = window_.getLogicalSize();
                    window_.setSceneViewport({0, 0, w, h});
                    imguiConsole_.setConsole(simulationController_.gameInstance()->developerConsole());
                    imguiConsole_.hide();
                    editorSettings_.disableGrid();
                } else {
                    inputSystem_.setBlocked(false);
                    window_.setSceneViewport(
                            computeSceneViewport(window_.getLogicalSize().first, window_.getLogicalSize().second));
                    imguiConsole_.setConsole(developerConsole_);
                    imguiConsole_.show();
                    editorSettings_.enableGrid();
                }
                simWasActive_ = simActive;
            }

            {
                ZoneScopedN("Input");
                inputSystem_.update();
                if (simActive) {
                    if (ImGui::IsKeyPressed(ImGuiKey_Backslash, false)) imguiConsole_.toggleVisibility();
                    inputSystem_.setBlocked(imguiConsole_.isVisible());
                }
                auto [mouseX, mouseY] = inputSystem_.getMousePosition();
                inputHandler_.update(mouseX, mouseY, deltaTime);
            }

            {
                ZoneScopedN("DeveloperConsole");
                developerConsole_.tick();
            }

            {
                ZoneScopedN("Render");
                if (simActive) {
                    simulationController_.tick(deltaTime);
                    editorRenderer_.render(simulationController_.world(), 0.0f, deltaTime);
                } else {
                    editorRenderer_.render(entityManager_, editorSettings_.getGridScale(), deltaTime);
                }
            }

            time_.endFrame();
            FrameMark;
        }
    }

private:
    enum class AppState { Welcome, Editor };

    static SceneViewport computeSceneViewport(int w, int h) {
        constexpr float HIERARCHY_WIDTH_RATIO = 0.15f;
        constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
        constexpr float ASSETS_HEIGHT_RATIO = 0.3f;
        int left = static_cast<int>(static_cast<float>(w) * HIERARCHY_WIDTH_RATIO);
        int right = static_cast<int>(static_cast<float>(w) * (1.0f - INSPECTOR_WIDTH_RATIO));
        int bottom = static_cast<int>(static_cast<float>(h) * (1.0f - ASSETS_HEIGHT_RATIO));
        return {left, 0, right - left, bottom};
    }

    void initApp() {
        setupViewport();
        editorSettings_.enableGrid();
        window_.onCloseRequest([this] {
            if (appState_ == AppState::Editor && undoHistory_.hasUnsavedChanges())
                closeModal_.show();
            else
                window_.requestClose();
        });
        closeModal_.setCallbacks({
                [this] {
                    project_.saveCurrentScene();
                    window_.requestClose();
                },
                [this] { window_.requestClose(); },
                [this] { /* cancel — do nothing */ },
        });
        imguiRoot_.setOverlayCallback([this] { closeModal_.draw(); });
        // If a project was already loaded (via --project CLI arg), skip the welcome screen
        if (project_.getCurrentScenePath().has_value()) enterEditor();
    }

    void enterEditor() {
        appState_ = AppState::Editor;
        editorRenderer_.setWelcomeMode(false);
        SceneViewport sv = window_.getSceneViewport();
        if (sv.width > 0 && sv.height > 0)
            editorCamera_.setAspectRatio(static_cast<float>(sv.width) / static_cast<float>(sv.height));
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
    UndoHistory& undoHistory_;
    InputHandler& inputHandler_;
    EditorRenderer& editorRenderer_;
    ImguiRoot& imguiRoot_;
    SimulationController& simulationController_;
    Imgui_Console& imguiConsole_;
    Imgui_WelcomeScreen& welcomeScreen_;
    Imgui_CloseModal closeModal_;
    AppState appState_ = AppState::Welcome;
    bool simWasActive_ = false;
};
