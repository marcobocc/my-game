#pragma once
#include <imgui.h>
#include <nfd.hpp>
#include <string>
#include "../../../business/ObjectSelection.hpp"
#include "../../../business/SceneLoader.hpp"
#include "../../../business/UndoHistory.hpp"
#include "../../../business/scene_editing/SceneMutations.hpp"
#include "../ImguiStyling.hpp"
#include "modules/ui/ImguiWidget.hpp"


class SceneLoader;
class ApplicationMenuBar : public ImguiWidget {
public:
    explicit ApplicationMenuBar(SceneMutations& sceneMutations,
                                SceneLoader& editorWorkspace,
                                UndoHistory& undoHistory,
                                ObjectSelection& objectSelection) :
        sceneMutations_(sceneMutations),
        editorWorkspace_(editorWorkspace),
        undoHistory_(undoHistory),
        objectSelection_(objectSelection) {
        editorWorkspace_.setOnSceneNameChanged([this](const std::string& name) { sceneName_ = name; });
        editorWorkspace_.setOnScenePathChanged([this](const std::filesystem::path& path) { scenePath_ = path; });
    }

    void draw() override {
        ImguiStyling::withMenuBar(ImVec4(0.08f, 0.08f, 0.08f, 1.00f), [this] {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Save", "Ctrl+S", false, scenePath_.has_value())) handleSave();
                if (ImGui::MenuItem("Save As...")) openSaveDialog();
                if (ImGui::MenuItem("Open...")) openLoadDialog();
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) undoHistory_.undo();
                if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) undoHistory_.redo();
                ImGui::Separator();
                drawObjectEditMenu();
                ImGui::EndMenu();
            }

            if (sceneName_) ImGui::TextDisabled("  %s", sceneName_->c_str());
        });
    }

private:
    SceneMutations& sceneMutations_;
    SceneLoader& editorWorkspace_;
    UndoHistory& undoHistory_;
    ObjectSelection& objectSelection_;
    std::optional<std::string> sceneName_{};
    std::optional<std::filesystem::path> scenePath_{};

    void handleSave() { editorWorkspace_.saveScene(scenePath_->string().c_str()); }

    void drawObjectEditMenu() {
        auto selectedId = objectSelection_.getSelectedEntityId();
        bool hasSelection = selectedId.has_value();

        const char* actionNames[] = {"Copy", "Cut", "Duplicate", "Paste", "Delete"};
        const char* shortcuts[] = {"Ctrl+C", "Ctrl+X", "Ctrl+D", "Ctrl+V", "Shift+X"};

        if (hasSelection) {
            auto actions = sceneMutations_.getObjectEditActions(*selectedId);
            int idx = 0;
            for (auto& [actionName, actionFunc]: actions) {
                if (ImGui::MenuItem(actionName.c_str(), shortcuts[idx])) {
                    actionFunc();
                }
                idx++;
            }
        } else {
            for (int i = 0; i < 5; ++i) {
                ImGui::MenuItem(actionNames[i], shortcuts[i], false, false);
            }
        }
    }

    void handleLoad() { editorWorkspace_.loadScene(scenePath_->string().c_str()); }

    void openSaveDialog() {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "scene"}};
        if (NFD::SaveDialog(outPath, filters, 1) == NFD_OKAY) {
            scenePath_ = outPath;
            handleSave();
            NFD::FreePath(outPath);
        }
    }

    void openLoadDialog() {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "scene"}};
        if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
            scenePath_ = outPath;
            handleLoad();
            NFD::FreePath(outPath);
        }
    }
};
