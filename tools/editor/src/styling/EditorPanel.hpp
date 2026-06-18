#pragma once
#include <imgui.h>


class EditorPanel {
public:
    virtual ~EditorPanel() = default;

protected:
    EditorPanel() = default;

    void draw(const char* title, ImGuiWindowFlags flags = 0) {
        ImGui::Begin(title, nullptr, flags);
        drawBody();
        ImGui::End();
    }

    virtual void drawBody() = 0;
};
