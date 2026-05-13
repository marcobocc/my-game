#pragma once
#include <imgui.h>
#include <nfd.hpp>
#include <string>
#include "../../../business/ActionDispatcher.hpp"
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/SceneLoader.hpp"
#include "../../../business/UndoHistory.hpp"
#include "../../../business/scene_editing/SceneMutations.hpp"
#include "../../../input/ShortcutBindingService.hpp"
#include "../ImguiStyling.hpp"
#include "modules/ui/ImguiWidget.hpp"


class SceneLoader;
class ApplicationMenuBar : public ImguiWidget {
public:
    explicit ApplicationMenuBar(SceneMutations& sceneMutations,
                                SceneLoader& editorWorkspace,
                                UndoHistory& undoHistory,
                                ObjectSelection& objectSelection,
                                ActionDispatcher& actionDispatcher,
                                ShortcutBindingService& shortcutBindingService) :
        sceneMutations_(sceneMutations),
        editorWorkspace_(editorWorkspace),
        undoHistory_(undoHistory),
        objectSelection_(objectSelection),
        actionDispatcher_(actionDispatcher),
        shortcutBindingService_(shortcutBindingService) {}

    void draw() override {
        ImguiStyling::withMenuBar(ImVec4(0.08f, 0.08f, 0.08f, 1.00f), [this] {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save",
                                    shortcutBindingService_.getShortcut(ActionID::SAVE).c_str(),
                                    false,
                                    editorWorkspace_.getScenePath().has_value())) {
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
            if (auto sceneName = editorWorkspace_.getSceneName()) {
                ImGui::TextDisabled("  %s", sceneName->c_str());
            }
        });
    }

private:
    SceneMutations& sceneMutations_;
    SceneLoader& editorWorkspace_;
    UndoHistory& undoHistory_;
    ObjectSelection& objectSelection_;
    ActionDispatcher& actionDispatcher_;
    ShortcutBindingService& shortcutBindingService_;

    void drawObjectEditMenu() const {
        auto selectedId = objectSelection_.getSelectedEntityId();
        bool hasSelection = selectedId.has_value();

        constexpr ActionID editActions[] = {
                ActionID::COPY, ActionID::CUT, ActionID::DUPLICATE, ActionID::PASTE, ActionID::DELETE};
        constexpr const char* actionNames[] = {"Copy", "Cut", "Duplicate", "Paste", "Delete"};

        for (int i = 0; i < 5; ++i) {
            std::string shortcutStr = shortcutBindingService_.getShortcut(editActions[i]);
            if (ImGui::MenuItem(actionNames[i], shortcutStr.c_str(), false, hasSelection)) {
                auto actions = sceneMutations_.getObjectEditActions(*selectedId);
                int idx = 0;
                for (auto& actionFunc: actions | std::views::values) {
                    if (idx == i) {
                        actionFunc();
                        break;
                    }
                    idx++;
                }
            }
        }
    }

    void openSaveDialog() const {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "scene"}};
        if (NFD::SaveDialog(outPath, filters, 1) == NFD_OKAY) {
            editorWorkspace_.saveScene(outPath);
            NFD::FreePath(outPath);
        }
    }

    void openLoadDialog() const {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "scene"}};
        if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
            editorWorkspace_.loadScene(outPath);
            NFD::FreePath(outPath);
        }
    }
};
