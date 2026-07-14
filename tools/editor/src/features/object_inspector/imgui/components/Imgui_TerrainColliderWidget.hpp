#pragma once
#include "../Imgui_InspectorWidget.hpp"

// TerrainCollider has no editable fields — the collision shape is always the
// entity's terrain mesh — so the widget only explains the behavior.
class Imgui_TerrainColliderWidget : public Imgui_InspectorWidget {
public:
    Imgui_TerrainColliderWidget() : Imgui_InspectorWidget("Terrain Collider") {}

private:
    void buildProperties() override {
        add<LabelProperty>("Shape", [] { return std::string("Terrain mesh heightfield"); });
    }
};
