#pragma once
#include <imgui.h>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"

class RuntimeScene;

class Imgui_ParticleEmitterWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_ParticleEmitterWidget(RuntimeScene& scene) :
        Imgui_InspectorWidget("ParticleEmitter", 12),
        scene_(scene) {}

    void setCurrentObjectId(EntityHandle id) { lastObjectId_.emplace(id); }
    void setComponent(const ParticleEmitter& c) {
        if (!editing_) component_ = c;
    }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    ParticleEmitter component_{};
    bool editing_ = false;
    std::string colorStartMinGroupId_ = UndoHistory::randomGroupId("Set Color Start Min");
    std::string colorStartMaxGroupId_ = UndoHistory::randomGroupId("Set Color Start Max");
    std::string colorEndMinGroupId_ = UndoHistory::randomGroupId("Set Color End Min");
    std::string colorEndMaxGroupId_ = UndoHistory::randomGroupId("Set Color End Max");

    void drawBody() override {
        if (!lastObjectId_) return;

        drawRow("Emission Rate", [&] {
            ImGui::DragFloat("##emissionRate", &component_.emissionRate, 1.0f, 0.0f, 1000.0f);
            trackDrag("Set Emission Rate");
        });
        drawRow("Lifetime Min", [&] {
            ImGui::DragFloat("##lifetimeMin", &component_.lifetimeMin, 0.05f, 0.01f, 60.0f);
            trackDrag("Set Lifetime Min");
        });
        drawRow("Lifetime Max", [&] {
            ImGui::DragFloat("##lifetimeMax", &component_.lifetimeMax, 0.05f, 0.01f, 60.0f);
            trackDrag("Set Lifetime Max");
        });
        drawRow("Size Start", [&] {
            ImGui::DragFloat("##sizeStart", &component_.sizeStart, 0.01f, 0.0f, 10.0f);
            trackDrag("Set Size Start");
        });
        drawRow("Size End", [&] {
            ImGui::DragFloat("##sizeEnd", &component_.sizeEnd, 0.01f, 0.0f, 10.0f);
            trackDrag("Set Size End");
        });

        // Cone shape
        ImGui::Separator();
        drawRow("Cone Angle", [&] {
            ImGui::DragFloat("##coneAngle", &component_.coneAngle, 0.5f, 0.0f, 90.0f);
            trackDrag("Set Cone Angle");
        });
        drawRow("Cone Radius", [&] {
            ImGui::DragFloat("##coneRadius", &component_.coneRadius, 0.01f, 0.0f, 100.0f);
            trackDrag("Set Cone Radius");
        });
        drawRow("Speed Min", [&] {
            ImGui::DragFloat("##speedMin", &component_.speedMin, 0.05f, 0.0f, 100.0f);
            trackDrag("Set Speed Min");
        });
        drawRow("Speed Max", [&] {
            ImGui::DragFloat("##speedMax", &component_.speedMax, 0.05f, 0.0f, 100.0f);
            trackDrag("Set Speed Max");
        });
        drawRow("Gravity", [&] {
            ImGui::DragFloat3("##gravity", &component_.gravity.x, 0.1f);
            trackDrag("Set Gravity");
        });

        // Color range
        ImGui::Separator();
        drawRow("Color Start Min", [&] {
            if (ImGui::ColorEdit4("##colorStartMin", &component_.colorStartMin.r)) commitEdit(colorStartMinGroupId_);
            if (ImGui::IsItemDeactivated()) {
                editing_ = false;
                colorStartMinGroupId_ = UndoHistory::randomGroupId("Set Color Start Min");
            } else if (ImGui::IsItemActive())
                editing_ = true;
        });
        drawRow("Color Start Max", [&] {
            if (ImGui::ColorEdit4("##colorStartMax", &component_.colorStartMax.r)) commitEdit(colorStartMaxGroupId_);
            if (ImGui::IsItemDeactivated()) {
                editing_ = false;
                colorStartMaxGroupId_ = UndoHistory::randomGroupId("Set Color Start Max");
            } else if (ImGui::IsItemActive())
                editing_ = true;
        });
        drawRow("Color End Min", [&] {
            if (ImGui::ColorEdit4("##colorEndMin", &component_.colorEndMin.r)) commitEdit(colorEndMinGroupId_);
            if (ImGui::IsItemDeactivated()) {
                editing_ = false;
                colorEndMinGroupId_ = UndoHistory::randomGroupId("Set Color End Min");
            } else if (ImGui::IsItemActive())
                editing_ = true;
        });
        drawRow("Color End Max", [&] {
            if (ImGui::ColorEdit4("##colorEndMax", &component_.colorEndMax.r)) commitEdit(colorEndMaxGroupId_);
            if (ImGui::IsItemDeactivated()) {
                editing_ = false;
                colorEndMaxGroupId_ = UndoHistory::randomGroupId("Set Color End Max");
            } else if (ImGui::IsItemActive())
                editing_ = true;
        });

        // Rotation range
        ImGui::Separator();
        drawRow("Rotation Min", [&] {
            ImGui::DragFloat("##rotationMin", &component_.rotationMin, 1.0f, -360.0f, 360.0f);
            trackDrag("Set Rotation Min");
        });
        drawRow("Rotation Max", [&] {
            ImGui::DragFloat("##rotationMax", &component_.rotationMax, 1.0f, -360.0f, 360.0f);
            trackDrag("Set Rotation Max");
        });

        // Color over lifetime gradient
        ImGui::Separator();
        ImGui::TextUnformatted("Color Over Lifetime");
        ImGui::Spacing();
        drawGradientEditor();

        // Other
        ImGui::Separator();
        drawRow("Max Particles", [&] {
            int mp = static_cast<int>(component_.maxParticles);
            if (ImGui::DragInt("##maxParticles", &mp, 1.0f, 1, 100000)) {
                component_.maxParticles = static_cast<uint32_t>(std::max(mp, 1));
                commitEdit(UndoHistory::randomGroupId("Set Max Particles"));
            }
        });
        drawRow("Sprite Texture", [&] {
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##spriteTexture",
                             const_cast<char*>(component_.spriteTexture.c_str()),
                             component_.spriteTexture.size() + 1,
                             ImGuiInputTextFlags_ReadOnly);
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("texture_asset")) {
                    component_.spriteTexture = static_cast<const char*>(payload->Data);
                    commitEdit(UndoHistory::randomGroupId("Set Sprite Texture"));
                }
                ImGui::EndDragDropTarget();
            }
        });
    }

    void drawGradientEditor() {
        int& count = component_.colorKeyCount;

        for (int i = 0; i < count; ++i) {
            ImGui::PushID(i);
            ParticleColorStop& stop = component_.colorKeys[i];

            bool isFirst = (i == 0);
            bool isLast = (i == count - 1);

            // Time — fixed at 0 and 1 for endpoints
            ImGui::SetNextItemWidth(55.0f);
            if (isFirst || isLast) {
                ImGui::BeginDisabled();
                ImGui::DragFloat("##t", &stop.time, 0.0f);
                ImGui::EndDisabled();
            } else {
                float tMin = component_.colorKeys[i - 1].time + 0.001f;
                float tMax = component_.colorKeys[i + 1].time - 0.001f;
                if (ImGui::DragFloat("##t", &stop.time, 0.005f, tMin, tMax))
                    commitEdit(UndoHistory::randomGroupId("Set Gradient Time"));
            }
            ImGui::SameLine();

            ImGui::SetNextItemWidth(160.0f);
            char colorId[16];
            snprintf(colorId, sizeof(colorId), "##gc%d", i);
            if (ImGui::ColorEdit4(colorId, &stop.color.r)) commitEdit(UndoHistory::randomGroupId("Set Gradient Color"));
            if (ImGui::IsItemDeactivated())
                editing_ = false;
            else if (ImGui::IsItemActive())
                editing_ = true;

            // Remove button — only for middle stops
            if (!isFirst && !isLast) {
                ImGui::SameLine();
                if (ImGui::SmallButton("-")) {
                    for (int j = i; j < count - 1; ++j)
                        component_.colorKeys[j] = component_.colorKeys[j + 1];
                    --count;
                    commitEdit(UndoHistory::randomGroupId("Remove Gradient Stop"));
                    ImGui::PopID();
                    break;
                }
            }
            ImGui::PopID();
        }

        if (count < PARTICLE_GRADIENT_MAX_STOPS) {
            if (ImGui::SmallButton("+ Stop")) {
                // Insert a new stop between the last two stops
                int last = count - 1;
                float newTime = (component_.colorKeys[last - 1].time + component_.colorKeys[last].time) * 0.5f;
                glm::vec4 newColor = (component_.colorKeys[last - 1].color + component_.colorKeys[last].color) * 0.5f;
                // Shift last stop to make room
                component_.colorKeys[count] = component_.colorKeys[last];
                component_.colorKeys[last] = {newTime, newColor};
                ++count;
                commitEdit(UndoHistory::randomGroupId("Add Gradient Stop"));
            }
        }
    }

    template<typename Fn>
    void drawRow(const char* label, Fn&& fn) {
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 120.0f);
        ImGui::TextUnformatted(label);
        ImGui::NextColumn();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 0));
        float availWidth = ImGui::GetContentRegionAvail().x;
        ImGui::SetNextItemWidth(availWidth * 0.75f);
        fn();
        ImGui::PopStyleVar();
        ImGui::Columns(1);
    }

    void trackDrag(const char* label = "Edit Particle") {
        if (!lastObjectId_) return;
        if (ImGui::IsItemDeactivatedAfterEdit()) commitEdit(UndoHistory::randomGroupId(label));
    }

    void commitEdit(const std::string& groupId) {
        if (!lastObjectId_) return;
        ParticleEmitter snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<ParticleEmitter>([snapshot](ParticleEmitter& e) { e = snapshot; }, groupId);
    }
};
