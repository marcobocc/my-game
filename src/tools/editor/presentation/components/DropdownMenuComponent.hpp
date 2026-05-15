#pragma once
#include <functional>
#include <imgui.h>
#include <string>
#include <vector>

struct MenuItem {
    std::string label;
    std::string shortcut;
    bool enabled = true;
    std::function<void()> onItemClick;
};

class DropdownMenuComponent {
public:
    virtual ~DropdownMenuComponent() = default;

    void setMenuItems(std::vector<MenuItem> items) { items_ = std::move(items); }

    void draw(const char* popupId, bool contextMenu = false) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 8));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 6));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 4.0f);
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.14f, 0.14f, 0.14f, 0.80f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.30f, 0.80f));

        if (contextMenu ? ImGui::BeginPopupContextItem(popupId) : ImGui::BeginPopup(popupId)) {
            for (const auto& item: items_) {
                if (!item.enabled) ImGui::BeginDisabled();
                if (ImGui::MenuItem(item.label.c_str(), item.shortcut.empty() ? nullptr : item.shortcut.c_str())) {
                    if (item.onItemClick) item.onItemClick();
                }
                if (!item.enabled) ImGui::EndDisabled();
            }
            if (!items_.empty()) ImGui::Separator();
            drawBody();
            ImGui::EndPopup();
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
    }

protected:
    DropdownMenuComponent() = default;
    virtual void drawBody() {}

private:
    std::vector<MenuItem> items_;
};
