#pragma once
#include <optional>
#include "../Imgui_InspectorWidget.hpp"
#include "modules/scene/components/ParticleEmitter.hpp"

class RuntimeScene;

class Imgui_ParticleEmitterWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_ParticleEmitterWidget(RuntimeScene& scene) :
        Imgui_InspectorWidget("ParticleEmitter"),
        scene_(scene) {}

    void setCurrentObjectId(EntityHandle id) { lastObjectId_.emplace(id); }
    void setComponent(const ParticleEmitter& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    ParticleEmitter component_{};

    // ---- Gradient editor property (inline, bespoke UI) -------------------

    class GradientEditorProperty : public InspectorProperty {
    public:
        explicit GradientEditorProperty(Imgui_ParticleEmitterWidget& owner) : owner_(owner) {}

        void draw() override {
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextUnformatted("Color Over Lifetime");
            ImGui::Spacing();

            ParticleEmitter& e = owner_.component_;
            int& count = e.colorKeyCount;

            for (int i = 0; i < count; ++i) {
                ImGui::PushID(i);
                ParticleColorStop& stop = e.colorKeys[i];
                bool isFirst = (i == 0);
                bool isLast = (i == count - 1);

                ImGui::SetNextItemWidth(55.0f);
                if (isFirst || isLast) {
                    ImGui::BeginDisabled();
                    ImGui::DragFloat("##t", &stop.time, 0.0f);
                    ImGui::EndDisabled();
                } else {
                    float tMin = e.colorKeys[i - 1].time + 0.001f;
                    float tMax = e.colorKeys[i + 1].time - 0.001f;
                    if (ImGui::DragFloat("##t", &stop.time, 0.005f, tMin, tMax))
                        owner_.commit(UndoHistory::randomGroupId("Set Gradient Time"));
                }
                ImGui::SameLine();

                ImGui::SetNextItemWidth(160.0f);
                char colorId[16];
                snprintf(colorId, sizeof(colorId), "##gc%d", i);
                if (ImGui::ColorEdit4(colorId, &stop.color.r))
                    owner_.commit(UndoHistory::randomGroupId("Set Gradient Color"));

                if (!isFirst && !isLast) {
                    ImGui::SameLine();
                    if (ImGui::SmallButton("-")) {
                        for (int j = i; j < count - 1; ++j)
                            e.colorKeys[j] = e.colorKeys[j + 1];
                        --count;
                        owner_.commit(UndoHistory::randomGroupId("Remove Gradient Stop"));
                        ImGui::PopID();
                        break;
                    }
                }
                ImGui::PopID();
            }

            if (count < PARTICLE_GRADIENT_MAX_STOPS) {
                if (ImGui::SmallButton("+ Stop")) {
                    int last = count - 1;
                    float newTime = (e.colorKeys[last - 1].time + e.colorKeys[last].time) * 0.5f;
                    glm::vec4 newColor = (e.colorKeys[last - 1].color + e.colorKeys[last].color) * 0.5f;
                    e.colorKeys[count] = e.colorKeys[last];
                    e.colorKeys[last] = {newTime, newColor};
                    ++count;
                    owner_.commit(UndoHistory::randomGroupId("Add Gradient Stop"));
                }
            }
        }

    private:
        Imgui_ParticleEmitterWidget& owner_;
    };

    // ---- buildProperties -------------------------------------------------

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<DragFloatProperty>(
                "Emission Rate",
                [this] { return component_.emissionRate; },
                [this](float v) { component_.emissionRate = v; },
                1.0f,
                0.0f,
                1000.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Emission Rate")); });

        add<DragFloatProperty>(
                "Lifetime Min",
                [this] { return component_.lifetimeMin; },
                [this](float v) { component_.lifetimeMin = v; },
                0.05f,
                0.01f,
                60.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Lifetime Min")); });

        add<DragFloatProperty>(
                "Lifetime Max",
                [this] { return component_.lifetimeMax; },
                [this](float v) { component_.lifetimeMax = v; },
                0.05f,
                0.01f,
                60.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Lifetime Max")); });

        add<DragFloatProperty>(
                "Size Start",
                [this] { return component_.sizeStart; },
                [this](float v) { component_.sizeStart = v; },
                0.01f,
                0.0f,
                10.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Size Start")); });

        add<DragFloatProperty>(
                "Size End",
                [this] { return component_.sizeEnd; },
                [this](float v) { component_.sizeEnd = v; },
                0.01f,
                0.0f,
                10.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Size End")); });

        add<SeparatorProperty>();

        add<DragFloatProperty>(
                "Cone Angle",
                [this] { return component_.coneAngle; },
                [this](float v) { component_.coneAngle = v; },
                0.5f,
                0.0f,
                90.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Cone Angle")); });

        add<DragFloatProperty>(
                "Cone Radius",
                [this] { return component_.coneRadius; },
                [this](float v) { component_.coneRadius = v; },
                0.01f,
                0.0f,
                100.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Cone Radius")); });

        add<DragFloatProperty>(
                "Speed Min",
                [this] { return component_.speedMin; },
                [this](float v) { component_.speedMin = v; },
                0.05f,
                0.0f,
                100.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Speed Min")); });

        add<DragFloatProperty>(
                "Speed Max",
                [this] { return component_.speedMax; },
                [this](float v) { component_.speedMax = v; },
                0.05f,
                0.0f,
                100.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Speed Max")); });

        add<Vec3Property>(
                "Gravity",
                [this] { return component_.gravity; },
                [this](glm::vec3 v) { component_.gravity = v; },
                0.1f,
                nullptr,
                [this] { commit(UndoHistory::randomGroupId("Set Gravity")); });

        add<SeparatorProperty>();

        add<ColorEdit4Property>(
                "Color Start Min",
                [this] { return component_.colorStartMin; },
                [this](glm::vec4 v) {
                    component_.colorStartMin = v;
                    commit(UndoHistory::randomGroupId("Set Color Start Min"));
                });

        add<ColorEdit4Property>(
                "Color Start Max",
                [this] { return component_.colorStartMax; },
                [this](glm::vec4 v) {
                    component_.colorStartMax = v;
                    commit(UndoHistory::randomGroupId("Set Color Start Max"));
                });

        add<ColorEdit4Property>(
                "Color End Min",
                [this] { return component_.colorEndMin; },
                [this](glm::vec4 v) {
                    component_.colorEndMin = v;
                    commit(UndoHistory::randomGroupId("Set Color End Min"));
                });

        add<ColorEdit4Property>(
                "Color End Max",
                [this] { return component_.colorEndMax; },
                [this](glm::vec4 v) {
                    component_.colorEndMax = v;
                    commit(UndoHistory::randomGroupId("Set Color End Max"));
                });

        add<SeparatorProperty>();

        add<DragFloatProperty>(
                "Rotation Min",
                [this] { return component_.rotationMin; },
                [this](float v) { component_.rotationMin = v; },
                1.0f,
                -360.0f,
                360.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Rotation Min")); });

        add<DragFloatProperty>(
                "Rotation Max",
                [this] { return component_.rotationMax; },
                [this](float v) { component_.rotationMax = v; },
                1.0f,
                -360.0f,
                360.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Rotation Max")); });

        addRaw(std::make_unique<GradientEditorProperty>(*this));

        add<SeparatorProperty>();

        add<SliderIntProperty>(
                "Max Particles",
                [this] { return static_cast<int>(component_.maxParticles); },
                [this](int v) {
                    component_.maxParticles = static_cast<uint32_t>(std::max(v, 1));
                    commit(UndoHistory::randomGroupId("Set Max Particles"));
                },
                1,
                100000);

        add<InputTextProperty>(
                "Sprite Texture",
                [this] { return component_.spriteTexture; },
                "texture_asset",
                [this](const char* path) {
                    component_.spriteTexture = path;
                    commit(UndoHistory::randomGroupId("Set Sprite Texture"));
                });
    }

    void commit(const std::string& groupId) {
        if (!lastObjectId_) return;
        ParticleEmitter snapshot = component_;
        scene_.getObject(*lastObjectId_)
                .mutateComponent<ParticleEmitter>([snapshot](ParticleEmitter& e) { e = snapshot; }, groupId);
    }
};
