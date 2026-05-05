#pragma once
#include <imgui.h>
#include "data/components/BoxCollider.hpp"

class SceneMutations;

class BoxColliderWidget {
public:
    explicit BoxColliderWidget(SceneMutations& sceneMutations) : sceneMutations_(sceneMutations), lastObjectId_("") {}

    void setCurrentObjectId(const std::string& objectId) { lastObjectId_ = objectId; }

    void draw(BoxCollider& b) {
        if (!ImGui::BeginChild("BoxCollider", {0, childHeight(3)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "Box Collider");
        ImGui::Spacing();
        ImGui::DragFloat3("Center", &b.center.x, 0.01f);
        trackDrag("BoxCollider", b.center);
        ImGui::DragFloat3("Half Extents", &b.halfExtents.x, 0.01f);
        trackDrag("BoxCollider", b.halfExtents);
        ImGui::EndChild();
    }

private:
    SceneMutations& sceneMutations_;
    std::string lastObjectId_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }

    template<typename T>
    void trackDrag(const std::string& componentType, T& target) {
        if (ImGui::IsItemActivated()) {
            sceneMutations_.beginEditByType(lastObjectId_, componentType);
        }
        if (ImGui::IsItemDeactivatedAfterEdit()) {
            sceneMutations_.endEditByType(lastObjectId_, componentType);
        }
    }
};
