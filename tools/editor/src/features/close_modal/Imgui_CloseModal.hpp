#pragma once
#include <functional>
#include <imgui.h>

class Imgui_CloseModal {
public:
    struct Callbacks {
        std::function<void()> onSaveAndQuit;
        std::function<void()> onQuitWithoutSaving;
        std::function<void()> onCancel;
    };

    void setCallbacks(Callbacks cb) { callbacks_ = std::move(cb); }

    void show() { pending_ = true; }

    void draw() {
        if (!pending_) return;

        ImGuiViewport* vp = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(vp->Pos);
        ImGui::SetNextWindowSize(vp->Size);

        constexpr ImGuiWindowFlags hostFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                               ImGuiWindowFlags_NoSavedSettings |
                                               ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;

        ImGui::Begin("##CloseModalHost", nullptr, hostFlags);
        ImGui::OpenPopup("Unsaved Changes");

        ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Always);
        if (ImGui::BeginPopupModal(
                    "Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
            ImGui::TextWrapped("You have unsaved changes. Do you really want to quit?");
            ImGui::Spacing();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImVec2 buttonSize(100, 28);
            float totalWidth = buttonSize.x * 3 + ImGui::GetStyle().ItemSpacing.x * 2;
            ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - totalWidth) * 0.5f + ImGui::GetCursorPosX());

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.52f, 0.30f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.63f, 0.37f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.42f, 0.24f, 1.0f));
            if (ImGui::Button("Save & Quit", buttonSize)) {
                pending_ = false;
                ImGui::CloseCurrentPopup();
                if (callbacks_.onSaveAndQuit) callbacks_.onSaveAndQuit();
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::Button("Quit", buttonSize)) {
                pending_ = false;
                ImGui::CloseCurrentPopup();
                if (callbacks_.onQuitWithoutSaving) callbacks_.onQuitWithoutSaving();
            }

            ImGui::SameLine();
            if (ImGui::Button("Cancel", buttonSize)) {
                pending_ = false;
                ImGui::CloseCurrentPopup();
                if (callbacks_.onCancel) callbacks_.onCancel();
            }

            ImGui::Spacing();
            ImGui::EndPopup();
        }

        ImGui::End();
    }

private:
    bool pending_ = false;
    Callbacks callbacks_;
};
