#pragma once
#include <imgui.h>
#include <string>
#include "../../../services/EditorMode.hpp"
#include "../../../services/EditorSettings.hpp"
#include "../../../services/SimulationController.hpp"
#include "../gizmos/EditorGizmos.hpp"


class Imgui_SceneViewToolbar {
public:
    static constexpr float BAR_HEIGHT = 20.0f;

    Imgui_SceneViewToolbar(EditorSettings& editorSettings,
                           EditorGizmos& editorGizmos,
                           SimulationController& simulationController,
                           EditorModeService& editorMode) :
        editorSettings_(editorSettings),
        editorGizmos_(editorGizmos),
        simulationController_(simulationController),
        editorMode_(editorMode) {}

    // Call this from inside the dockspace host window, before DockSpace().
    void draw(float width) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {6.0f, 0.0f});

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddRectFilled(pos, {pos.x + width, pos.y + BAR_HEIGHT}, IM_COL32(20, 20, 20, 255));

        ImGui::BeginChild("ToolbarRow",
                          ImVec2(width, BAR_HEIGHT),
                          false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8.0f);

        ImGui::AlignTextToFramePadding();

        ImGui::Text("Scene");

        ImGui::SameLine();
        ImGui::Dummy({20.0f, 0.0f});
        ImGui::SameLine();
        drawModeDropdown();

        ImGui::SameLine();
        ImGui::Dummy({20.0f, 0.0f});
        ImGui::SameLine();

        auto drawButton = [&](const char* label,
                              bool enabled,
                              auto callback,
                              ImVec4 enabledColor = ImVec4{0.2f, 0.5f, 0.8f, 1.0f}) {
            ImVec4 color = enabled ? enabledColor : ImVec4{0.4f, 0.4f, 0.4f, 1.0f};

            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color);

            bool pressed = ImGui::Button(label);

            ImGui::PopStyleColor(3);

            if (pressed) callback();
        };

        const char* transformMode = editorSettings_.isLocalTransformEnabled() ? "Local Transform" : "World Transform";
        drawButton(transformMode, editorSettings_.isLocalTransformEnabled(), [&] {
            editorSettings_.toggleLocalTransform();
        });
        ImGui::SameLine();
        drawButton(
                "Snap",
                editorSettings_.isGridSnappingEnabled(),
                [&] { editorSettings_.toggleSnapping(); },
                ImVec4{0.8f, 0.4f, 0.0f, 1.0f});

        // Centre: Play button
        ImGui::SameLine();
        {
            float availWidth = ImGui::GetContentRegionAvail().x;
            float playW = ImGui::CalcTextSize("> Play").x + ImGui::GetStyle().FramePadding.x * 2 + 12.0f;
            float rightGroupW = ImGui::CalcTextSize("Grid").x + ImGui::GetStyle().FramePadding.x * 2 +
                                ImGui::CalcTextSize("Lighting").x + ImGui::GetStyle().FramePadding.x * 2 +
                                ImGui::CalcTextSize("BVH").x + ImGui::GetStyle().FramePadding.x * 2 +
                                ImGui::GetStyle().ItemSpacing.x * 2;
            float centreOffset = (availWidth - rightGroupW - playW) * 0.5f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + centreOffset);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.55f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.70f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.3f, 0.80f, 0.3f, 1.0f});
            if (ImGui::Button("> Play")) simulationController_.requestPlay();
            ImGui::PopStyleColor(3);
        }

        // Right-align remaining buttons
        ImGui::SameLine();
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float buttonWidths = ImGui::CalcTextSize("Grid").x + ImGui::GetStyle().FramePadding.x * 2 +
                             ImGui::CalcTextSize("Lighting").x + ImGui::GetStyle().FramePadding.x * 2 +
                             ImGui::CalcTextSize("BVH").x + ImGui::GetStyle().FramePadding.x * 2 +
                             ImGui::GetStyle().ItemSpacing.x * 2;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + availableWidth - buttonWidths);

        drawButton("Grid", editorSettings_.isGridEnabled(), [&] { editorSettings_.toggleGrid(); });
        ImGui::SameLine();
        drawButton("Lighting", editorSettings_.isLightingEnabled(), [&] { editorSettings_.toggleLighting(); });
        ImGui::SameLine();
        drawButton("BVH", editorGizmos_.bvhEnabled(), [&] { editorGizmos_.toggleBVH(); });

        ImGui::EndChild();
        ImGui::PopStyleVar(2);
    }

private:
    static const char* modeLabel(EditorMode mode) {
        switch (mode) {
            case EditorMode::Terrain:
                return "Terrain";
            case EditorMode::MeshBuilder:
                return "Mesh Builder";
            default:
                return "Selection";
        }
    }

    void drawModeDropdown() {
        const EditorMode mode = editorMode_.mode();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.25f, 0.25f, 0.28f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.35f, 0.35f, 0.4f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.35f, 0.35f, 0.4f, 1.0f});
        std::string label = std::string("Mode: ") + modeLabel(mode);
        if (ImGui::Button(label.c_str())) ImGui::OpenPopup("##ModeDropdown");
        ImGui::PopStyleColor(3);

        if (ImGui::BeginPopup("##ModeDropdown")) {
            if (ImGui::Selectable("Selection", mode == EditorMode::Selection))
                editorMode_.setMode(EditorMode::Selection);
            if (ImGui::Selectable("Terrain", mode == EditorMode::Terrain)) editorMode_.setMode(EditorMode::Terrain);
            if (ImGui::Selectable("Mesh Builder", mode == EditorMode::MeshBuilder))
                editorMode_.setMode(EditorMode::MeshBuilder);
            ImGui::EndPopup();
        }
    }

    EditorSettings& editorSettings_;
    EditorGizmos& editorGizmos_;
    SimulationController& simulationController_;
    EditorModeService& editorMode_;
};
