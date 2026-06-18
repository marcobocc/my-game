#pragma once
#include <functional>
#include <imgui.h>
#include <imgui_internal.h>
#include "../../../../runtime/src/modules/console/Imgui_Console.hpp"
#include "application_toolbar/Imgui_ApplicationMenuBar.hpp"
#include "asset_browser/imgui/Imgui_AssetGridPanel.hpp"
#include "modules/core/GameWindow.hpp"
#include "object_inspector/imgui/Imgui_InspectorPanel.hpp"
#include "scene_hierarchy/imgui/Imgui_HierarchyPanel.hpp"
#include "scene_viewport/imgui/Imgui_EditorSceneViewport.hpp"
#include "scene_viewport/scene_toolbar/Imgui_SceneViewToolbar.hpp"
#include "scene_viewport/scene_toolbar/Imgui_SimHUD.hpp"
#include "welcome/Imgui_WelcomeScreen.hpp"

class ImguiRoot {
public:
    ImguiRoot(Imgui_ApplicationMenuBar& applicationMenuBar,
              Imgui_SceneViewToolbar& sceneViewToolbar,
              Imgui_HierarchyPanel& hierarchyPanel,
              Imgui_AssetGridPanel& assetGridPanel,
              Imgui_InspectorPanel& inspectorPanel,
              Imgui_EditorSceneViewport& editorSceneViewport,
              Imgui_Console& console,
              GameWindow& window) :
        applicationMenuBar_(applicationMenuBar),
        sceneViewToolbar_(sceneViewToolbar),
        hierarchyPanel_(hierarchyPanel),
        assetGridPanel_(assetGridPanel),
        inspectorPanel_(inspectorPanel),
        editorSceneViewport_(editorSceneViewport),
        console_(console),
        window_(window) {}

    void setOverlayCallback(std::function<void()> cb) { overlayCallback_ = std::move(cb); }

    void draw() {
        applicationMenuBar_.draw();
        drawDockSpace();

        hierarchyPanel_.draw();
        assetGridPanel_.draw();
        inspectorPanel_.draw();
        editorSceneViewport_.draw();
        console_.draw();
        if (overlayCallback_) overlayCallback_();
    }

private:
    void drawDockSpace() {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImVec2 dockPos = {viewport->WorkPos.x, viewport->WorkPos.y};
        ImVec2 dockSize = {viewport->WorkSize.x, viewport->WorkSize.y};
        ImGui::SetNextWindowPos(dockPos);
        ImGui::SetNextWindowSize(dockSize);
        ImGui::SetNextWindowViewport(viewport->ID);

        constexpr ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                               ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                                               ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDocking;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0.137f, 0.165f, 0.204f, 1.0f)); // rgb(35 42 52)
        ImGui::Begin("##DockSpaceHost", nullptr, hostFlags);
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        sceneViewToolbar_.draw(dockSize.x);

        ImGuiID dockId = ImGui::GetID("##MainDockSpace");
        ImGui::DockSpace(dockId,
                         ImVec2(dockSize.x, dockSize.y - Imgui_SceneViewToolbar::BAR_HEIGHT),
                         ImGuiDockNodeFlags_PassthruCentralNode);

        if (!layoutBuilt_) {
            buildDefaultLayout(dockId, viewport->WorkSize);
            layoutBuilt_ = true;
        }

        // Read the central (passthrough) node rect and update the scene viewport every frame.
        // centralNode->Pos is in desktop coordinates with ViewportsEnable; subtract the OS window
        // origin (viewport->Pos) to get coordinates relative to the GLFW framebuffer.
        if (ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockId)) {
            ImVec2 winOrigin = viewport->Pos;
            ImVec2 pos = centralNode->Pos;
            ImVec2 size = centralNode->Size;
            centralNodePos_ = pos;
            centralNodeSize_ = size;
            window_.setSceneViewport({static_cast<int>(pos.x - winOrigin.x),
                                      static_cast<int>(pos.y - winOrigin.y),
                                      static_cast<int>(size.x),
                                      static_cast<int>(size.y)});
        }

        ImGui::End();
    }

    static void buildDefaultLayout(ImGuiID dockId, ImVec2 size) {
        ImGui::DockBuilderRemoveNode(dockId);
        ImGui::DockBuilderAddNode(dockId, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockId, size);

        // Split left (hierarchy) | rest
        ImGuiID dockLeft, dockCenter;
        ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Left, 0.15f, &dockLeft, &dockCenter);

        // Split rest: center | right (inspector)
        ImGuiID dockRight;
        ImGui::DockBuilderSplitNode(dockCenter, ImGuiDir_Right, 0.25f / 0.85f, &dockRight, &dockCenter);

        // Split center: scene (passthru) | bottom strip — full width across center+left+right
        // We split the whole dockId for the bottom to get full width
        ImGuiID dockMain, dockBottom;
        ImGui::DockBuilderSplitNode(dockId, ImGuiDir_Down, 0.28f, &dockBottom, &dockMain);

        // Re-dock left/right/center into the upper portion
        ImGuiID dockUpperLeft, dockUpperCenter;
        ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.15f, &dockUpperLeft, &dockUpperCenter);
        ImGuiID dockUpperRight;
        ImGui::DockBuilderSplitNode(dockUpperCenter, ImGuiDir_Right, 0.25f / 0.85f, &dockUpperRight, &dockUpperCenter);

        // Split bottom: asset grid (left, ~75%) | console (right, ~25%)
        ImGuiID dockGrid, dockConsole;
        ImGui::DockBuilderSplitNode(dockBottom, ImGuiDir_Right, 0.28f, &dockConsole, &dockGrid);

        ImGui::DockBuilderDockWindow("Objects Hierarchy", dockUpperLeft);
        ImGui::DockBuilderDockWindow("Inspector", dockUpperRight);
        ImGui::DockBuilderDockWindow("Asset Grid", dockGrid);
        ImGui::DockBuilderDockWindow("Console", dockConsole);

        ImGui::DockBuilderFinish(dockId);
    }

    Imgui_ApplicationMenuBar& applicationMenuBar_;
    Imgui_SceneViewToolbar& sceneViewToolbar_;
    Imgui_HierarchyPanel& hierarchyPanel_;
    Imgui_AssetGridPanel& assetGridPanel_;
    Imgui_InspectorPanel& inspectorPanel_;
    Imgui_EditorSceneViewport& editorSceneViewport_;
    Imgui_Console& console_;
    GameWindow& window_;
    std::function<void()> overlayCallback_;
    ImVec2 centralNodePos_{0.0f, 0.0f};
    ImVec2 centralNodeSize_{0.0f, 0.0f};
    bool layoutBuilt_ = false;
};

class SimHUDRoot {
public:
    SimHUDRoot(Imgui_SimHUD& simHUD, Imgui_Console& console) : simHUD_(simHUD), console_(console) {}
    void draw() {
        simHUD_.draw();
        console_.draw();
    }

private:
    Imgui_SimHUD& simHUD_;
    Imgui_Console& console_;
};

class WelcomeRoot {
public:
    explicit WelcomeRoot(Imgui_WelcomeScreen& screen) : screen_(screen) {}
    void draw() { screen_.draw(); }

private:
    Imgui_WelcomeScreen& screen_;
};
