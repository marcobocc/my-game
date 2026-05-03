#pragma once
#include <functional>
#include <imgui.h>
#include <optional>
#include <string>
#include "systems/ui/ImguiWidget.hpp"

class EditorMenuBar : public ImguiWidget {
public:
    std::function<void()> onSave;
    std::function<void()> onSaveAs;
    std::function<void()> onOpen;
    std::function<void()> onUndo;
    std::function<void()> onRedo;
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

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z"))
                if (onUndo) onUndo();
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z"))
                if (onRedo) onRedo();
            ImGui::EndMenu();
        }

        if (sceneName) ImGui::TextDisabled("  %s", sceneName->c_str());

        ImGui::EndMainMenuBar();
    }
};
