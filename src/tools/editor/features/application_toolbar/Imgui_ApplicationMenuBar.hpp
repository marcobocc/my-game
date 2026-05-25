#pragma once
#include <imgui.h>
#include <nfd.hpp>
#include <string>
#include "../../services/ActionDispatcher.hpp"
#include "../../services/EditorContext.hpp"
#include "../../services/EditorSelection.hpp"
#include "../../services/UndoHistory.hpp"
#include "../../styling/ImguiStyling.hpp"
#include "../input_handling/ShortcutBindingService.hpp"


class Imgui_ApplicationMenuBar {
public:
    explicit Imgui_ApplicationMenuBar(EditorContext& project,
                                      UndoHistory& undoHistory,
                                      EditorSelection& editorSelection,
                                      ActionDispatcher& actionDispatcher,
                                      ShortcutBindingService& shortcutBindingService) :
        project_(project),
        undoHistory_(undoHistory),
        editorSelection_(editorSelection),
        actionDispatcher_(actionDispatcher),
        shortcutBindingService_(shortcutBindingService) {}

    void draw() {
        ImguiStyling::withMenuBar(ImVec4(0.08f, 0.08f, 0.08f, 1.00f), [this] {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save",
                                    shortcutBindingService_.getShortcut(ActionID::SAVE).c_str(),
                                    false,
                                    project_.getCurrentScenePath().has_value())) {
                    actionDispatcher_.execute(ActionID::SAVE);
                }
                if (ImGui::MenuItem("Save As...")) openSaveDialog();
                if (ImGui::MenuItem("Open...")) openLoadDialog();
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", shortcutBindingService_.getShortcut(ActionID::UNDO).c_str()))
                    undoHistory_.undo();
                if (ImGui::MenuItem("Redo", shortcutBindingService_.getShortcut(ActionID::REDO).c_str()))
                    undoHistory_.redo();
                ImGui::Separator();
                drawObjectEditMenu();
                ImGui::EndMenu();
            }
            if (auto path = project_.getCurrentScenePath()) {
                ImGui::TextDisabled("  %s", path->filename().string().c_str());
            }
        });
    }

private:
    EditorContext& project_;
    UndoHistory& undoHistory_;
    EditorSelection& editorSelection_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;

    void drawObjectEditMenu() const {
        bool hasSelection = !editorSelection_.getSelectedEntityIds().empty();

        constexpr ActionID editActions[] = {
                ActionID::COPY, ActionID::CUT, ActionID::DUPLICATE, ActionID::PASTE, ActionID::DELETE};
        constexpr const char* actionNames[] = {"Copy", "Cut", "Duplicate", "Paste", "Delete"};

        for (int i = 0; i < 5; ++i) {
            std::string shortcutStr = shortcutBindingService_.getShortcut(editActions[i]);
            if (ImGui::MenuItem(actionNames[i], shortcutStr.c_str(), false, hasSelection)) {
                actionDispatcher_.execute(editActions[i]);
            }
        }
    }

    void openSaveDialog() const {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "scene"}};
        if (NFD::SaveDialog(outPath, filters, 1) == NFD_OKAY) {
            project_.saveScene(outPath);
            NFD::FreePath(outPath);
        }
    }

    void openLoadDialog() const {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "scene"}};
        if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
            project_.loadScene(outPath);
            NFD::FreePath(outPath);
        }
    }
};
