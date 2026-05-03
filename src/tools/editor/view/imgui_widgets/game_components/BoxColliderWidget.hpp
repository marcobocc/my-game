#pragma once
#include <imgui.h>
#include "../../../controller/SceneMutationsController.hpp"
#include "data/components/BoxCollider.hpp"

class BoxColliderWidget {
public:
    explicit BoxColliderWidget(SceneMutationsController& mutations) : mutations_(mutations) {}

    void draw(BoxCollider& b) const {
        if (!ImGui::BeginChild("BoxCollider", {0, childHeight(3)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Box Collider");
        ImGui::Spacing();
        ImGui::DragFloat3("Center", &b.center.x, 0.01f);
        trackDrag(b.center);
        ImGui::DragFloat3("Half Extents", &b.halfExtents.x, 0.01f);
        trackDrag(b.halfExtents);
        ImGui::EndChild();
    }

private:
    SceneMutationsController& mutations_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    template<typename T>
    void trackDrag(T& target) const {
        if (ImGui::IsItemActivated()) mutations_.beginEdit(target);
        if (ImGui::IsItemDeactivatedAfterEdit()) mutations_.commitEdit(target);
    }
};
