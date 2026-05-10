#pragma once
#include <functional>
#include <imgui.h>

class ComponentContainer {
public:
    ComponentContainer(const char* title, int rows) : title_(title), rows_(rows) {}

    void draw(std::function<void()> contents) {
        if (!ImGui::BeginChild(title_, {0, childHeight(rows_)}, true)) {
            ImGui::EndChild();
            return;
        }
        ImGui::TextColored({0.8f, 0.7f, 0.2f, 1.0f}, "%s", title_);
        ImGui::Spacing();
        contents();
        ImGui::EndChild();
    }

    void drawWithContextMenu(const char* contextId, std::function<void()> contents, std::function<void()> onRemove) {
        if (!ImGui::BeginChild(title_, {0, childHeight(rows_)}, true)) {
            ImGui::EndChild();
            return;
        }

        ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() - windowPadding.x);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - windowPadding.y);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {windowPadding.x, windowPadding.y * 0.5f});
        ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.2f, 0.2f, 0.5f});
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.2f, 0.2f, 0.5f});
        ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, {0.0f, 0.5f});
        ImGui::Button(title_, {ImGui::GetContentRegionAvail().x + windowPadding.x, ImGui::GetFrameHeight()});
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(2);

        if (ImGui::BeginPopupContextItem(contextId)) {
            if (ImGui::MenuItem("Remove")) {
                onRemove();
            }
            ImGui::EndPopup();
        }
        ImGui::Spacing();
        contents();
        ImGui::EndChild();
    }

private:
    const char* title_;
    int rows_;

    static float childHeight(int rows) {
        return ImGui::GetFrameHeightWithSpacing() * static_cast<float>(rows) + ImGui::GetStyle().WindowPadding.y * 2.0f;
    }
};
