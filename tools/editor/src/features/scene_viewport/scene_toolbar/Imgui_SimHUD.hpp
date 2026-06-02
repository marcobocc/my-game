#pragma once
#include <imgui.h>
#include "../../../services/SimulationController.hpp"

class Imgui_SimHUD {
public:
    explicit Imgui_SimHUD(SimulationController& sim) : sim_(sim) {}

    void draw() {
        handleKeyBindings();
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        constexpr float BAR_HEIGHT = 32.0f;
        constexpr float BAR_WIDTH = 360.0f;

        float xPos = (viewport->Size.x - BAR_WIDTH) * 0.5f;
        ImGui::SetNextWindowPos({viewport->Pos.x + xPos, viewport->Pos.y + 8.0f});
        ImGui::SetNextWindowSize({BAR_WIDTH, BAR_HEIGHT});
        ImGui::SetNextWindowBgAlpha(0.75f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                           ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                           ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {6.0f, 4.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6.0f, 3.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {6.0f, 0.0f});

        ImGui::Begin("##SimHUD", nullptr, flags);

        if (sim_.isRunning()) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.7f, 0.5f, 0.0f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.8f, 0.6f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.9f, 0.7f, 0.2f, 1.0f});
            if (ImGui::Button("|| Pause  [Esc]")) sim_.pause();
            ImGui::PopStyleColor(3);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.6f, 0.1f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            if (ImGui::Button(">  Resume  [Esc]")) sim_.resume();
            ImGui::PopStyleColor(3);
        }

        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.7f, 0.1f, 0.1f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.8f, 0.2f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.9f, 0.3f, 0.3f, 1.0f});
        if (ImGui::Button("[] Stop  [Shift+Esc]")) sim_.stop();
        ImGui::PopStyleColor(3);

        ImGui::End();
        ImGui::PopStyleVar(3);
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
