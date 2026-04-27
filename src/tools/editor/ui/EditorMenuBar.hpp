#pragma once
#include <functional>
#include <imgui.h>
#include <optional>
#include <string>
#include "core/ui/ImguiWidget.hpp"

class EditorMenuBar : public ImguiWidget {
public:
    std::function<void()> onSave;
    std::function<void()> onSaveAs;
    std::function<void()> onOpen;
    std::optional<std::string> sceneName{};

    void draw() const override {
        if (!ImGui::BeginMainMenuBar()) return;

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S", false, onSave && sceneName.has_value()))
                if (onSave) onSave();
            if (ImGui::MenuItem("Save As..."))
                if (onSaveAs) onSaveAs();
            if (ImGui::MenuItem("Open..."))
                if (onOpen) onOpen();
            ImGui::EndMenu();
        }

        if (sceneName) ImGui::TextDisabled("  %s", sceneName->c_str());

        ImGui::EndMainMenuBar();
    }
};
