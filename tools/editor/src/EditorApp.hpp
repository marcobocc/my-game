#pragma once
#include <filesystem>
#include <imgui.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include <tracy/Tracy.hpp>
#include "../../../runtime/src/console/ConsoleLogAppender.hpp"
#include "../../../runtime/src/graphics/debug/Imgui_Console.hpp"
#include "animation/AnimationSystem.hpp"
#include "console/DeveloperConsole.hpp"
#include "core/GameWindow.hpp"
#include "core/TimeManager.hpp"
#include "core/scene/World.hpp"
#include "features/ImguiRoot.hpp"
#include "features/close_modal/Imgui_CloseModal.hpp"
#include "features/input_handling/InputHandler.hpp"
#include "features/mesh_builder/MeshBuilderTool.hpp"
#include "features/scene_viewport/editor_camera/EditorCamera.hpp"
#include "features/terrain_tool/TerrainPaintTool.hpp"
#include "features/terrain_tool/TerrainSculptTool.hpp"
#include "features/welcome/Imgui_WelcomeScreen.hpp"
#include "input/InputSystem.hpp"
#include "physics/PhysicsSystem.hpp"
#include "rendering/EditorRenderer.hpp"
#include "services/AssetThumbnailGenerator.hpp"
#include "services/EditorContext.hpp"
#include "services/EditorMode.hpp"
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
              Imgui_WelcomeScreen& welcomeScreen,
              AnimationSystem& animationSystem,
              AssetThumbnailGenerator& thumbnailGenerator,
              TerrainSculptTool& terrainSculptTool,
              TerrainPaintTool& terrainPaintTool,
              MeshBuilderTool& meshBuilderTool,
              EditorModeService& editorMode) :
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
        welcomeScreen_(welcomeScreen),
        animationSystem_(animationSystem),
        thumbnailGenerator_(thumbnailGenerator),
        terrainSculptTool_(terrainSculptTool),
        terrainPaintTool_(terrainPaintTool),
        meshBuilderTool_(meshBuilderTool),
        editorMode_(editorMode) {
        welcomeScreen_.setCallbacks({
                [this](const std::filesystem::path& path) { openProject(path); },
                [this] { newProject(); },
        });
        editorRenderer_.setAnimationSystem(&animationSystem_);
        editorRenderer_.setDebugOverlayCallback([this](DebugDraw& debugDraw) {
            terrainSculptTool_.update(debugDraw);
            terrainPaintTool_.update(debugDraw);
            meshBuilderTool_.update(debugDraw);
        });
        // Leaving a tool's mode cancels any in-flight stroke, same as the old tab-switch behavior.
        editorMode_.subscribe([this](EditorMode mode) {
            if (mode != EditorMode::Terrain) {
                terrainSculptTool_.setActive(false);
                terrainPaintTool_.setActive(false);
            }
            if (mode != EditorMode::MeshBuilder) meshBuilderTool_.setActive(false);
        });
        initApp();
        ImguiStyling::ApplyEditorStyle();
    }

    void openProject(const std::filesystem::path& projectRoot) {
        project_.openProject(projectRoot);
        simulationController_.setProjectRoot(projectRoot);
        thumbnailGenerator_.setProjectRoot(projectRoot);
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
            simulationController_.flushPendingPlay();

            bool simActive = simulationController_.isActive();
            if (simActive != simWasActive_) {
                editorRenderer_.setSimMode(simActive);
                if (simActive) {
                    editorMode_.setMode(EditorMode::Selection);
                    auto [w, h] = window_.getLogicalSize();
                    window_.setSceneViewport({0, 0, w, h});
                    imguiConsole_.setConsole(simulationController_.gameInstance()->developerConsole());
                    imguiConsole_.hide();
                    editorSettings_.disableGrid();
                    editorRenderer_.setAnimationSystem(&simulationController_.gameInstance()->animationSystem());
                    editorRenderer_.setPlayModeDebugDraw(&simulationController_.gameInstance()->debugDraw());
                    editorRenderer_.setPlayModeUISource(&simulationController_.gameInstance()->uiSystem(), &window_);
                } else {
                    inputSystem_.setBlocked(false);
                    window_.setSceneViewport(
                            computeSceneViewport(window_.getLogicalSize().first, window_.getLogicalSize().second));
                    imguiConsole_.setConsole(developerConsole_);
                    imguiConsole_.show();
                    editorSettings_.enableGrid();
                    editorRenderer_.setAnimationSystem(&animationSystem_);
                    editorRenderer_.setPlayModeDebugDraw(nullptr);
                    editorRenderer_.setPlayModeUISource(nullptr, nullptr);
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
                inputHandler_.setViewportToolMode(!simActive && (editorMode_.mode() == EditorMode::Terrain ||
                                                                 editorMode_.mode() == EditorMode::MeshBuilder));
                inputHandler_.update(mouseX, mouseY, deltaTime, simActive);
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
                    animationSystem_.update(deltaTime, entityManager_);
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
    AnimationSystem& animationSystem_;
    AssetThumbnailGenerator& thumbnailGenerator_;
    TerrainSculptTool& terrainSculptTool_;
    TerrainPaintTool& terrainPaintTool_;
    MeshBuilderTool& meshBuilderTool_;
    EditorModeService& editorMode_;
    Imgui_CloseModal closeModal_;
    AppState appState_ = AppState::Welcome;
    bool simWasActive_ = false;
};
