#pragma once
#include <imgui.h>
#include "../../../business/EditorGizmos.hpp"
#include "../../../business/EditorSettings.hpp"
#include "modules/ui/ImguiWidget.hpp"

class SceneViewToolbar : public ImguiWidget {
public:
    static constexpr float HIERARCHY_WIDTH_RATIO = 0.15f;
    static constexpr float INSPECTOR_WIDTH_RATIO = 0.25f;
    static constexpr float BAR_HEIGHT = 20.0f;

    SceneViewToolbar(EditorSettings& editorSettings, EditorGizmos& editorGizmos) :
        editorSettings_(editorSettings),
        editorGizmos_(editorGizmos) {}

    void draw() override {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float menuBarHeight = ImGui::GetFrameHeight() * 0.5f;
        float width = viewport->Size.x * (1.0f - HIERARCHY_WIDTH_RATIO - INSPECTOR_WIDTH_RATIO);
        float xPos = viewport->Size.x * HIERARCHY_WIDTH_RATIO;

        ImGui::SetNextWindowPos({viewport->Pos.x + xPos, viewport->Pos.y + menuBarHeight});
        ImGui::SetNextWindowSize({width, BAR_HEIGHT});

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {8.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.0f, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {6.0f, 0.0f});

        ImGui::Begin("SceneViewToolbar", nullptr, flags);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);

        ImVec2 p0 = ImGui::GetWindowPos();
        ImVec2 sz = ImGui::GetWindowSize();
        ImGui::GetWindowDrawList()->AddRectFilled(
                p0, {p0.x + sz.x, p0.y + sz.y}, ImGui::GetColorU32({0.05f, 0.05f, 0.05f, 1.0f}), 4.0f);

        ImGui::BeginChild(
                "ToolbarRow", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4.0f);

        ImGui::AlignTextToFramePadding();

        ImGui::Text("Scene");

        ImGui::SameLine();
        ImGui::Dummy({40.0f, 0.0f});
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

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

private:
    EditorSettings& editorSettings_;
    EditorGizmos& editorGizmos_;
};
