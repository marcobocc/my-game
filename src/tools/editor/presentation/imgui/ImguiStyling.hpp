#pragma once
#include <imgui.h>

class ImguiStyling {
public:
    static void ApplyEditorStyle() {
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;

        style.WindowPadding = ImVec2(0, 0);
        style.FramePadding = ImVec2(6, 2);
        style.ItemSpacing = ImVec2(6, 2);
        style.ItemInnerSpacing = ImVec2(4, 2);
        style.IndentSpacing = 16.0f;
        style.FrameRounding = 4.0f;
        style.WindowRounding = 0.0f;
        style.ChildRounding = 0.0f;
        style.ScrollbarSize = 12.0f;
        style.GrabMinSize = 8.0f;
        style.ChildBorderSize = 0.0f;
        style.WindowBorderSize = 0.0f;
        style.FrameBorderSize = 0.0f;

        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.2f, 0.2f, 0.2f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.14f, 0.95f);
        colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.00f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.45f, 0.45f, 0.45f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.95f, 0.65f, 0.25f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
        colors[ImGuiCol_Header] = ImVec4(0.3f, 0.5f, 0.8f, 0.8f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 0.3f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.3f, 0.5f, 0.8f, 0.8f);
        colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.25f, 0.50f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.50f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.25f, 0.25f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.35f, 0.35f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.22f, 0.24f, 1.00f);
        colors[ImGuiCol_TabActive] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.82f, 0.55f, 0.20f, 0.25f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.95f, 0.95f, 0.95f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.95f, 0.65f, 0.25f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.82f, 0.55f, 0.20f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(0.82f, 0.55f, 0.20f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.82f, 0.55f, 0.20f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    template<typename Fn>
    static void withBodyStyling(Fn&& fn) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 2.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 10.0f));
        fn();
        ImGui::PopStyleVar(3);
    }

    template<typename Fn>
    static void withPopup(const char* name, Fn&& fn, bool contextMenu = false) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.14f, 0.14f, 0.14f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.30f, 0.80f));
        if ((!contextMenu && ImGui::BeginPopup(name)) || contextMenu && ImGui::BeginPopupContextItem(name)) {
            fn();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    template<typename Fn>
    static void withPopupModal(const char* name, Fn&& fn) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.14f, 0.14f, 0.14f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.30f, 0.80f));
        if (ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            fn();
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

    template<typename Fn>
    static void withMenuBar(ImVec4 bgColor, Fn&& fn) {
        ImGui::PushStyleColor(ImGuiCol_MenuBarBg, bgColor);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.20f, 0.20f, 1.0f));

        if (!ImGui::BeginMainMenuBar()) return;
        fn();
        ImGui::EndMainMenuBar();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
};
