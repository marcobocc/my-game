#pragma once
#include <imgui.h>
#include "modules/ui/ImguiWidget.hpp"

class EditorPanel : public ImguiWidget {
public:
    ~EditorPanel() override = default;

protected:
    EditorPanel() = default;

    void draw(const char* title, ImVec2 position, ImVec2 size, ImGuiWindowFlags flags = 0) {
        ImGui::SetNextWindowPos(position);
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowBgAlpha(0.95f);

        constexpr ImGuiWindowFlags defaultFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        ImGui::Begin(title, nullptr, defaultFlags | flags);

        drawHeader(title);
        drawBody();

        ImGui::End();
    }

    virtual void drawBody() = 0;

private:
    static void drawHeader(const char* title) {
        constexpr float height = 20.0f;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
        ImGui::BeginChild("##header",
                          {0, height},
                          ImGuiChildFlags_AlwaysUseWindowPadding,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::TextColored(ImVec4(0.8f, 0.7f, 0.2f, 1.0f), "%s", title);
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
};
