#pragma once
#include <imgui.h>
#include <nfd.hpp>
#include <string>
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
                                UndoHistory& undoHistory) :
        sceneMutations_(sceneMutations),
        editorWorkspace_(editorWorkspace),
        undoHistory_(undoHistory) {}

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
                ImGui::EndMenu();
            }

            if (sceneName_) ImGui::TextDisabled("  %s", sceneName_->c_str());
        });
    }

private:
    SceneMutations& sceneMutations_;
    SceneLoader& editorWorkspace_;
    UndoHistory& undoHistory_;
    std::optional<std::string> sceneName_{};
    std::optional<std::filesystem::path> scenePath_{};

    void handleSave() { editorWorkspace_.saveScene(scenePath_->string().c_str()); }

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
