#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <string>
#include "core/objects/components/Material.hpp"
#include "core/objects/components/Transform.hpp"
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

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) +
               ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    void drawInspectorPanel() const {
        auto& obj = scene_->getObject(*currentActiveObjectId_);


        if (ImGui::BeginChild("TransformPanel", ImVec2(0, childHeight(5)), true)) {
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Transform");
            ImGui::Spacing();
            if (obj.has<Transform>()) {
                auto& t = obj.get<Transform>();
                ImGui::DragFloat3("Position", &t.position.x, 0.01f);
                glm::vec3 euler = glm::degrees(glm::eulerAngles(t.rotation));
                if (ImGui::DragFloat3("Rotation", &euler.x, 0.5f)) t.rotation = glm::quat(glm::radians(euler));
                ImGui::DragFloat3("Scale", &t.scale.x, 0.01f);
            } else {
                ImGui::TextDisabled("No transform");
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();

        if (ImGui::BeginChild("MaterialPanel", ImVec2(0, childHeight(4)), true)) {
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Material");
            ImGui::Spacing();
            if (obj.has<Material>()) {
                auto& mat = obj.get<Material>();
                float col[4] = {mat.baseColor.r, mat.baseColor.g, mat.baseColor.b, mat.baseColor.a};
                if (ImGui::ColorEdit4("Base Color", col)) mat.baseColor = glm::vec4(col[0], col[1], col[2], col[3]);
                if (!mat.textureName.empty())
                    ImGui::Text("Texture: %s", mat.textureName.c_str());
                else
                    ImGui::TextDisabled("Texture: none");
            } else {
                ImGui::TextDisabled("No material");
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();

        // header row + spacing row + name row
        if (ImGui::BeginChild("MeshPanel", ImVec2(0, childHeight(3)), true)) {
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Mesh");
            ImGui::Spacing();
            if (obj.has<Mesh>()) {
                const auto& mesh = obj.get<Mesh>();
                ImGui::Text("Name: %s", mesh.name.c_str());
            } else {
                ImGui::TextDisabled("No mesh");
            }
        }
        ImGui::EndChild();
    }
};
