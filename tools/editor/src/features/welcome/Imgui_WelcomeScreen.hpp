#pragma once
#include <algorithm>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

class Imgui_WelcomeScreen {
public:
    struct Callbacks {
        std::function<void(std::filesystem::path)> onOpenProject;
        std::function<void()> onNewProject;
    };

    explicit Imgui_WelcomeScreen(std::filesystem::path projectsRoot, std::filesystem::path samplesRoot = {}) :
        projectsRoot_(std::move(projectsRoot)),
        samplesRoot_(std::move(samplesRoot)) {
        scanProjects();
    }

    void setCallbacks(Callbacks callbacks) { callbacks_ = std::move(callbacks); }

    void draw() {
        ImGuiViewport* vp = ImGui::GetMainViewport();
        constexpr float PANEL_W = 480.0f;
        constexpr float PANEL_H = 380.0f;
        float x = (vp->Size.x - PANEL_W) * 0.5f;
        float y = (vp->Size.y - PANEL_H) * 0.5f;

        ImGui::SetNextWindowPos({vp->Pos.x + x, vp->Pos.y + y});
        ImGui::SetNextWindowSize({PANEL_W, PANEL_H});
        ImGui::SetNextWindowBgAlpha(0.96f);

        constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                           ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoSavedSettings;

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {20.0f, 20.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8.0f, 8.0f});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::Begin("##WelcomeScreen", nullptr, flags);

        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Game Engine");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.5f, 0.2f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.3f, 0.6f, 0.3f, 1.0f});
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.4f, 0.7f, 0.4f, 1.0f});
        if (ImGui::Button("New Project", {PANEL_W - 40.0f, 32.0f})) {
            callbacks_.onNewProject();
        }
        ImGui::PopStyleColor(3);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        const bool hasSamples = !samples_.empty();
        const float listH = hasSamples ? (PANEL_H - 160.0f) * 0.5f : 0.0f;

        if (projects_.empty()) {
            ImGui::TextDisabled("No projects found in %s", projectsRoot_.string().c_str());
        } else {
            ImGui::TextDisabled("Projects");
            ImGui::Spacing();
            ImGui::BeginChild(
                    "##projects", {0.0f, hasSamples ? listH : 0.0f}, ImGuiChildFlags_None, ImGuiWindowFlags_None);
            for (const auto& p: projects_) {
                if (ImGui::Selectable(p.filename().string().c_str())) {
                    callbacks_.onOpenProject(p);
                }
            }
            ImGui::EndChild();
        }

        if (hasSamples) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextDisabled("Samples");
            ImGui::Spacing();
            ImGui::BeginChild("##samples", {0.0f, 0.0f}, ImGuiChildFlags_None, ImGuiWindowFlags_None);
            for (const auto& p: samples_) {
                if (ImGui::Selectable(p.filename().string().c_str())) {
                    callbacks_.onOpenProject(p);
                }
            }
            ImGui::EndChild();
        }

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

private:
    void scanProjects() {
        auto scan = [](const std::filesystem::path& root, std::vector<std::filesystem::path>& out) {
            out.clear();
            if (!std::filesystem::exists(root)) return;
            for (const auto& entry: std::filesystem::directory_iterator(root)) {
                if (entry.is_directory()) out.push_back(entry.path());
            }
            std::sort(out.begin(), out.end());
        };
        scan(projectsRoot_, projects_);
        scan(samplesRoot_, samples_);
    }

    std::filesystem::path projectsRoot_;
    std::filesystem::path samplesRoot_;
    Callbacks callbacks_;
    std::vector<std::filesystem::path> projects_;
    std::vector<std::filesystem::path> samples_;
};
