#pragma once
#include <imgui.h>
#include <string>
#include "core/objects/components/Material.hpp"
#include "core/scene/Scene.hpp"
#include "core/ui/ImguiWidget.hpp"

class InspectorSidePanel : public ImguiWidget {
public:
    InspectorSidePanel(std::string* currentActiveObjectId, Scene* scene) :
        currentActiveObjectId_(currentActiveObjectId),
        scene_(scene) {}

    void draw() const override { drawDockspace(); }

private:
    std::string* currentActiveObjectId_;
    Scene* scene_;

    void drawDockspace() const {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float sidebarWidth = 300.0f;
        ImVec2 sidebarPos = ImVec2(viewport->Pos.x + viewport->Size.x - sidebarWidth, viewport->Pos.y);
        ImVec2 sidebarSize = ImVec2(sidebarWidth, viewport->Size.y);

        ImGui::SetNextWindowPos(sidebarPos);
        ImGui::SetNextWindowSize(sidebarSize);
        ImGui::SetNextWindowBgAlpha(0.95f);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                                 ImGuiWindowFlags_NoNavFocus;

        ImGui::Begin("SidebarDock", nullptr, flags);

        float objectsPanelHeight = 120.0f;
        if (ImGui::BeginChild("InspectorPanel", ImVec2(0, 0), true)) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Inspector");
            ImGui::Spacing();
            drawInspectorPanel();
        }
        ImGui::EndChild();
        ImGui::End();
    }

    void drawInspectorPanel() const {
        auto& obj = scene_->getObject(*currentActiveObjectId_);
        auto& mat = obj.get<Material>();

        float materialPanelHeight = 90.0f;
        if (ImGui::BeginChild("MaterialPanel", ImVec2(0, materialPanelHeight), true)) {
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Material");
            ImGui::Spacing();
            float col[4] = {mat.baseColor.r, mat.baseColor.g, mat.baseColor.b, mat.baseColor.a};
            if (ImGui::ColorEdit4("Base Color", col)) {
                mat.baseColor = glm::vec4(col[0], col[1], col[2], col[3]);
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();
        if (ImGui::BeginChild("MeshPanel", ImVec2(0, 0), true)) {
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Mesh");
            ImGui::Spacing();
            const auto& mesh = obj.get<Mesh>();
            ImGui::Text("Name: %s", mesh.name.c_str());
        }
        ImGui::EndChild();
    }
};
