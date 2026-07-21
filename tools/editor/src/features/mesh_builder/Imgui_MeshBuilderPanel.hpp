#pragma once
#include <imgui.h>
#include "../../services/EditorSelection.hpp"
#include "../../styling/EditorPanel.hpp"
#include "MeshBuilderTool.hpp"

class Imgui_MeshBuilderPanel : public EditorPanel {
public:
    Imgui_MeshBuilderPanel(MeshBuilderTool& tool, EditorSelection& selection) : tool_(tool), selection_(selection) {}

    void draw() { EditorPanel::draw("Mesh Builder"); }

protected:
    void drawBody() override {
        tool_.setActive(true);

        if (selection_.getSelectedEntityIds().empty()) {
            ImGui::TextDisabled("Select a mesh's entity to edit.");
            return;
        }

        ImGui::Text("Select");
        MeshBuilderSelectMode mode = tool_.selectMode();

        bool isVertex = mode == MeshBuilderSelectMode::Vertex;
        if (isVertex) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.8f, 1.0f});
        if (ImGui::Button("Vertex")) tool_.setSelectMode(MeshBuilderSelectMode::Vertex);
        if (isVertex) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isEdge = mode == MeshBuilderSelectMode::Edge;
        if (isEdge) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.8f, 1.0f});
        if (ImGui::Button("Edge")) tool_.setSelectMode(MeshBuilderSelectMode::Edge);
        if (isEdge) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isFace = mode == MeshBuilderSelectMode::Face;
        if (isFace) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.8f, 1.0f});
        if (ImGui::Button("Face")) tool_.setSelectMode(MeshBuilderSelectMode::Face);
        if (isFace) ImGui::PopStyleColor();

        ImGui::Separator();
        const char* elementName = isVertex ? "vertex" : (isEdge ? "edge" : "face");
        ImGui::TextWrapped("Click a %s to select it, then drag to move it.", elementName);
    }

private:
    MeshBuilderTool& tool_;
    EditorSelection& selection_;
};
