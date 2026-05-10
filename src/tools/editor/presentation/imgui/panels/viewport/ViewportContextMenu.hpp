#pragma once
#include <functional>
#include <imgui.h>
#include <optional>
#include "../../../../../../engine/modules/ui/ImguiWidget.hpp"
#include "../../ImguiStyling.hpp"
#include "../hierarchy/HierarchyDropdownMenu.hpp"
#include "../hierarchy/SpherePopupModal.hpp"

class SceneMutations;
class AssetManager;

class ViewportContextMenu : public ImguiWidget {
public:
    ViewportContextMenu(AssetManager& assetManager,
                        SceneMutations& sceneMutations,
                        std::function<void(uint32_t)> onSphereCreated = nullptr) :
        spherePopupModal_(onSphereCreated),
        dropdownMenu_(assetManager, sceneMutations, &spherePopupModal_),
        openContextMenu(false),
        rightClickStartX_(0.0f),
        rightClickStartY_(0.0f),
        hasMovedSinceRightClick_(false) {}

    void draw() override {
        spherePopupModal_.draw();

        // Track right-click without drag
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !ImGui::IsAnyItemHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            rightClickStartX_ = mousePos.x;
            rightClickStartY_ = mousePos.y;
            hasMovedSinceRightClick_ = false;
        }

        // Check if mouse moved since right-click
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
            ImVec2 mousePos = ImGui::GetMousePos();
            float dx = mousePos.x - rightClickStartX_;
            float dy = mousePos.y - rightClickStartY_;
            if (dx * dx + dy * dy > 25.0f) { // threshold of 5 pixels
                hasMovedSinceRightClick_ = true;
            }
        }

        // Open popup only if right-click released without significant movement
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right) && !hasMovedSinceRightClick_) {
            openContextMenu = true;
        }

        if (openContextMenu) {
            ImGui::OpenPopup("ViewportContextMenu");
            openContextMenu = false;
        }

        ImguiStyling::withPopup("ViewportContextMenu", [&] { dropdownMenu_.draw(std::nullopt); });
    }

private:
    SpherePopupModal spherePopupModal_;
    HierarchyDropdownMenu dropdownMenu_;
    bool openContextMenu;
    float rightClickStartX_;
    float rightClickStartY_;
    bool hasMovedSinceRightClick_;
};
