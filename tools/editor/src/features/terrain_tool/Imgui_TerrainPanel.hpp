#pragma once
#include <algorithm>
#include <imgui.h>
#include <utility>
#include "../../services/EditorSelection.hpp"
#include "../../services/common_editing/SceneQuickActions.hpp"
#include "../../styling/EditorPanel.hpp"
#include "TerrainPaintTool.hpp"
#include "TerrainSculptTool.hpp"

class Imgui_TerrainPanel : public EditorPanel {
public:
    Imgui_TerrainPanel(TerrainSculptTool& sculptTool,
                       TerrainPaintTool& paintTool,
                       SceneQuickActions& sceneQuickActions,
                       EditorSelection& selection) :
        sculptTool_(sculptTool),
        paintTool_(paintTool),
        sceneQuickActions_(sceneQuickActions),
        selection_(selection) {}

    void draw() { EditorPanel::draw("Terrain"); }

protected:
    void drawBody() override {
        // Only one of Sculpt/Paint is ever engaged at a time (they share left-click-drag
        // in the viewport); picking a brush button below switches the active tool.
        sculptTool_.setActive(activeTool_ == ActiveTool::Sculpt);
        paintTool_.setActive(activeTool_ == ActiveTool::Paint);

        ImGui::Text("Sculpt");
        static const std::pair<const char*, TerrainBrushOp> sculptBrushes[] = {
                {"Raise", TerrainBrushOp::RAISE},
                {"Lower", TerrainBrushOp::LOWER},
                {"Smooth", TerrainBrushOp::SMOOTH},
                {"Flatten", TerrainBrushOp::FLATTEN},
        };
        TerrainBrushOp currentOp = sculptTool_.brushOp();
        for (size_t i = 0; i < IM_ARRAYSIZE(sculptBrushes); ++i) {
            const auto& [label, op] = sculptBrushes[i];
            bool isCurrent = activeTool_ == ActiveTool::Sculpt && op == currentOp;
            if (isCurrent) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.8f, 1.0f});
            if (ImGui::Button(label)) {
                activeTool_ = ActiveTool::Sculpt;
                sculptTool_.setBrushOp(op);
            }
            if (isCurrent) ImGui::PopStyleColor();
            if (i + 1 < IM_ARRAYSIZE(sculptBrushes)) ImGui::SameLine();
        }

        if (activeTool_ == ActiveTool::Sculpt) {
            float radius = sculptTool_.radius();
            if (ImGui::SliderFloat("Radius", &radius, 0.5f, 20.0f)) sculptTool_.setRadius(radius);
            float strength = sculptTool_.strength();
            if (ImGui::SliderFloat("Strength", &strength, 0.1f, 10.0f)) sculptTool_.setStrength(strength);
        }

        ImGui::Separator();
        ImGui::Text("Paint");

        auto selected = selection_.getSelectedEntityIds();
        if (selected.empty()) {
            ImGui::TextDisabled("Select a terrain mesh's entity to paint.");
            return;
        }

        if (ImGui::Button("Enable Painting")) {
            sceneQuickActions_.enableTerrainPainting(selected.front(), splatMapResolution_);
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(80);
        ImGui::InputInt("Splatmap Res", &splatMapResolution_);
        splatMapResolution_ = std::clamp(splatMapResolution_, 4, 2048);

        static const char* layerNames[] = {"Layer 0", "Layer 1", "Layer 2", "Layer 3"};
        int currentLayer = paintTool_.activeLayer();
        for (int layer = 0; layer < IM_ARRAYSIZE(layerNames); ++layer) {
            bool isCurrent = activeTool_ == ActiveTool::Paint && layer == currentLayer;
            if (isCurrent) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.5f, 0.8f, 1.0f});
            if (ImGui::Button(layerNames[layer])) {
                activeTool_ = ActiveTool::Paint;
                paintTool_.setActiveLayer(layer);
            }
            if (isCurrent) ImGui::PopStyleColor();
            if (layer + 1 < IM_ARRAYSIZE(layerNames)) ImGui::SameLine();
        }

        if (activeTool_ == ActiveTool::Paint) {
            float radius = paintTool_.radius();
            if (ImGui::SliderFloat("Radius", &radius, 0.5f, 20.0f)) paintTool_.setRadius(radius);
            float strength = paintTool_.strength();
            if (ImGui::SliderFloat("Strength", &strength, 0.1f, 10.0f)) paintTool_.setStrength(strength);
        }
    }

private:
    enum class ActiveTool { Sculpt, Paint };

    TerrainSculptTool& sculptTool_;
    TerrainPaintTool& paintTool_;
    SceneQuickActions& sceneQuickActions_;
    EditorSelection& selection_;
    ActiveTool activeTool_ = ActiveTool::Sculpt;
    int splatMapResolution_ = 512;
};
