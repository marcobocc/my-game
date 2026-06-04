#pragma once
#include <imgui.h>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/Light.hpp"

class RuntimeScene;

class Imgui_LightWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_LightWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Light", 8), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const Light& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    Light component_{};

    void drawBody() override {
        if (!lastObjectId_) return;

        drawRow("Type", [&] {
            const char* items[] = {"Directional", "Spot"};
            int currentType = static_cast<int>(component_.type);
            if (ImGui::Combo("##type", &currentType, items, IM_ARRAYSIZE(items))) {
                component_.type = static_cast<LightType>(currentType);
                commitEdit(UndoHistory::randomGroupId("Set Type"));
            }
        });

        drawRow("Color", [&] {
            float col[3] = {component_.color.r, component_.color.g, component_.color.b};
            if (ImGui::ColorEdit3("##color", col)) {
                component_.color = glm::vec3(col[0], col[1], col[2]);
                commitEdit(UndoHistory::randomGroupId("Set Color"));
            }
        });

        drawRow("Intensity", [&] {
            ImGui::DragFloat("##intensity", &component_.intensity, 0.1f, 0.0f, 10.0f);
            trackDrag();
        });

        drawRow("Cast Shadows", [&] {
            bool v = component_.castShadows;
            if (ImGui::Checkbox("##castShadows", &v)) {
                component_.castShadows = v;
                commitEdit(UndoHistory::randomGroupId("Set Cast Shadows"));
            }
        });

        if (component_.type == LightType::SPOT) {
            drawRow("Inner Angle", [&] {
                ImGui::DragFloat("##innerAngle", &component_.innerConeAngle, 0.5f, 0.0f, 89.0f);
                trackDrag("Set Inner Angle");
            });

            drawRow("Outer Angle", [&] {
                ImGui::DragFloat("##outerAngle", &component_.outerConeAngle, 0.5f, 0.0f, 90.0f);
                trackDrag("Set Outer Angle");
            });

            drawRow("Range", [&] {
                ImGui::DragFloat("##range", &component_.range, 0.5f, 0.0f, 1000.0f);
                trackDrag("Set Range");
            });
        }
    }

    template<typename Fn>
    void drawRow(const char* label, Fn&& fn) {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 90.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(availWidth * 0.5f);
        fn();
        ImGui::PopStyleVar();
        ImGui::Columns(1);
    }

    void trackDrag(const char* label = "Set Intensity") {
        if (!lastObjectId_) return;
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            commitEdit(UndoHistory::randomGroupId(label));
        }
    }

    void commitEdit(const std::string& groupId) {
        if (!lastObjectId_) return;
        Light snapshot = component_;
        scene_.getObject(*lastObjectId_).mutateComponent<Light>([snapshot](Light& l) { l = snapshot; }, groupId);
    }
};
