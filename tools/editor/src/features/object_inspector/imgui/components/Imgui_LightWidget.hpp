#pragma once
#include <optional>
#include "../../../../../../../runtime/src/graphics/components/Light.hpp"
#include "../Imgui_InspectorWidget.hpp"

class RuntimeScene;

class Imgui_LightWidget : public Imgui_InspectorWidget {
public:
    explicit Imgui_LightWidget(RuntimeScene& scene) : Imgui_InspectorWidget("Light"), scene_(scene) {}

    void setCurrentObjectId(EntityHandle objectId) { lastObjectId_.emplace(objectId); }
    void setComponent(const Light& c) { component_ = c; }

private:
    RuntimeScene& scene_;
    std::optional<EntityHandle> lastObjectId_;
    Light component_{};

    void buildProperties() override {
        if (!lastObjectId_) return;

        add<ComboProperty>(
                "Type",
                std::vector<std::string>{"Directional", "Spot", "Point"},
                [this] { return static_cast<int>(component_.type); },
                [this](int v) {
                    component_.type = static_cast<LightType>(v);
                    commit(UndoHistory::randomGroupId("Set Type"));
                });

        add<ColorEdit3Property>(
                "Color",
                [this] { return component_.color; },
                [this](glm::vec3 v) {
                    component_.color = v;
                    commit(UndoHistory::randomGroupId("Set Color"));
                });

        add<DragFloatProperty>(
                "Intensity",
                [this] { return component_.intensity; },
                [this](float v) { component_.intensity = v; },
                0.1f,
                0.0f,
                10.0f,
                [this] { commit(UndoHistory::randomGroupId("Set Intensity")); });

        add<CheckboxProperty>(
                "Cast Shadows",
                [this] { return component_.castShadows; },
                [this](bool v) {
                    component_.castShadows = v;
                    commit(UndoHistory::randomGroupId("Set Cast Shadows"));
                });

        if (component_.type == LightType::SPOT) {
            add<DragFloatProperty>(
                    "Inner Angle",
                    [this] { return component_.innerConeAngle; },
                    [this](float v) { component_.innerConeAngle = v; },
                    0.5f,
                    0.0f,
                    89.0f,
                    [this] { commit(UndoHistory::randomGroupId("Set Inner Angle")); });

            add<DragFloatProperty>(
                    "Outer Angle",
                    [this] { return component_.outerConeAngle; },
                    [this](float v) { component_.outerConeAngle = v; },
                    0.5f,
                    0.0f,
                    90.0f,
                    [this] { commit(UndoHistory::randomGroupId("Set Outer Angle")); });

            add<DragFloatProperty>(
                    "Range",
                    [this] { return component_.range; },
                    [this](float v) { component_.range = v; },
                    0.5f,
                    0.0f,
                    1000.0f,
                    [this] { commit(UndoHistory::randomGroupId("Set Range")); });
        }

        if (component_.type == LightType::POINT) {
            add<DragFloatProperty>(
                    "Range",
                    [this] { return component_.range; },
                    [this](float v) { component_.range = v; },
                    0.5f,
                    0.0f,
                    1000.0f,
                    [this] { commit(UndoHistory::randomGroupId("Set Range")); });
        }
    }

    void commit(const std::string& groupId) {
        if (!lastObjectId_) return;
        Light snapshot = component_;
        scene_.getObject(*lastObjectId_).mutateComponent<Light>([snapshot](Light& l) { l = snapshot; }, groupId);
    }
};
