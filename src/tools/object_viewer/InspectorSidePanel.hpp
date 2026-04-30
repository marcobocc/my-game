#pragma once
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <string>
#include "assets/types/Material.hpp"
#include "core/GameEngine.hpp"
#include "core/objects/components/Renderer.hpp"
#include "core/objects/components/Transform.hpp"
#include "core/ui/ImguiWidget.hpp"

class InspectorSidePanel : public ImguiWidget {
public:
    InspectorSidePanel(std::string* currentActiveObjectId, GameEngine& engine, float sceneWidthRatio) :
        currentActiveObjectId_(currentActiveObjectId),
        engine_(engine),
        sceneWidthRatio_(sceneWidthRatio) {}

    void draw() const override { drawDockspace(); }

private:
    std::string* currentActiveObjectId_;
    GameEngine& engine_;
    float sceneWidthRatio_;

    void drawDockspace() const {
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        float sidebarWidth = viewport->Size.x * (1.0f - sceneWidthRatio_);
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
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    void drawInspectorPanel() const {
        auto& obj = engine_.getObject(*currentActiveObjectId_);

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

        if (ImGui::BeginChild("RendererPanel", ImVec2(0, childHeight(5)), true)) {
            ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "Renderer");
            ImGui::Spacing();
            if (obj.has<Renderer>()) {
                auto& renderer = obj.get<Renderer>();

                ImGui::Text("Mesh: %s", renderer.meshName.c_str());

                const Material* mat = engine_.getAsset<Material>(renderer.materialName);
                ImGui::Text("Material: %s", renderer.materialName.c_str());

                if (!mat->getTextureName().empty())
                    ImGui::Text("Texture: %s", mat->getTextureName().c_str());
                else
                    ImGui::TextDisabled("Texture: none");

                glm::vec4 color = renderer.baseColorOverride.value_or(mat->getBaseColor());
                float col[4] = {color.r, color.g, color.b, color.a};
                if (ImGui::ColorEdit4("Base color", col))
                    renderer.baseColorOverride = glm::vec4(col[0], col[1], col[2], col[3]);
            } else {
                ImGui::TextDisabled("No renderer");
            }
        }
        ImGui::EndChild();
        ImGui::Spacing();
    }
};
