#pragma once
#include <imgui.h>
#include "../../../services/SimulationController.hpp"

class Imgui_SimHUD {
public:
    explicit Imgui_SimHUD(SimulationController& sim) : sim_(sim) {}

    void draw() {
        handleKeyBindings();

        constexpr float BTN_PADDING_X = 14.0f;
        constexpr float BTN_HEIGHT = 24.0f;
        constexpr float BAR_PADDING_X = 10.0f;
        constexpr float BAR_PADDING_Y = 6.0f;
        constexpr float ROUNDING = 8.0f;
        constexpr float BTN_SPACING = 6.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {BAR_PADDING_X, BAR_PADDING_Y});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.0f, 4.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {BTN_SPACING, 0.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, ROUNDING);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, ROUNDING * 0.5f);

        const char* playPauseLabel = sim_.isRunning() ? "|| Pause  [Esc]" : ">  Resume  [Esc]";
        const char* stopLabel = "[] Stop  [Shift+Esc]";

        float btnW1 = ImGui::CalcTextSize(playPauseLabel).x + BTN_PADDING_X * 2;
        float btnW2 = ImGui::CalcTextSize(stopLabel).x + BTN_PADDING_X * 2;

        float barWidth = BAR_PADDING_X * 2 + btnW1 + BTN_SPACING + btnW2;
        float barHeight = BAR_PADDING_Y * 2 + BTN_HEIGHT;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float xPos = (viewport->Size.x - barWidth) * 0.5f;
        ImGui::SetNextWindowPos({viewport->Pos.x + xPos, viewport->Pos.y + 10.0f});
        ImGui::SetNextWindowSize({barWidth, barHeight});
        ImGui::SetNextWindowBgAlpha(0.85f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                           ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("##SimHUD", nullptr, flags);

        if (sim_.isRunning()) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.70f, 0.50f, 0.00f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.80f, 0.60f, 0.10f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.90f, 0.70f, 0.20f, 1.0f});
            if (ImGui::Button(playPauseLabel, {btnW1, BTN_HEIGHT})) sim_.pause();
            ImGui::PopStyleColor(3);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.10f, 0.55f, 0.10f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.18f, 0.68f, 0.18f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.25f, 0.78f, 0.25f, 1.0f});
            if (ImGui::Button(playPauseLabel, {btnW1, BTN_HEIGHT})) sim_.resume();
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.65f, 0.10f, 0.10f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.78f, 0.18f, 0.18f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.90f, 0.28f, 0.28f, 1.0f});
        if (ImGui::Button(stopLabel, {btnW2, BTN_HEIGHT})) sim_.stop();
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(5);
    }

private:
    SimulationController& sim_;

    void handleKeyBindings() {
        if (!sim_.isActive()) return;

        bool shiftHeld = ImGui::IsKeyDown(ImGuiKey_LeftShift) || ImGui::IsKeyDown(ImGuiKey_RightShift);

        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (shiftHeld) {
                sim_.stop();
            } else if (sim_.isRunning()) {
                sim_.pause();
            } else if (sim_.isPaused()) {
                sim_.resume();
            }
        }
    }
};
