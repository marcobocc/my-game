#pragma once
#include <imgui.h>
#include <nfd.hpp>
#include <string>
#include "../../../business/SceneLoader.hpp"
#include "../../../business/SceneMutations.hpp"
#include "systems/ui/ImguiWidget.hpp"


class SceneLoader;
class EditorMenuBar : public ImguiWidget {
public:
    explicit EditorMenuBar(SceneMutations& sceneMutations, SceneLoader& editorWorkspace) :
        sceneMutations_(sceneMutations),
        editorWorkspace_(editorWorkspace) {}

    void draw() override {
        if (!ImGui::BeginMainMenuBar()) return;

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save", "Ctrl+S", false, scenePath_.has_value())) handleSave();
            if (ImGui::MenuItem("Save As...")) openSaveDialog();
            if (ImGui::MenuItem("Open...")) openLoadDialog();
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) sceneMutations_.undoHistory().undo();
            if (ImGui::MenuItem("Redo", "Ctrl+Shift+Z")) sceneMutations_.undoHistory().redo();
            ImGui::EndMenu();
        }

        if (sceneName_) ImGui::TextDisabled("  %s", sceneName_->c_str());
        ImGui::EndMainMenuBar();
    }

private:
    SceneMutations& sceneMutations_;
    SceneLoader& editorWorkspace_;
    std::optional<std::string> sceneName_{};
    std::optional<std::filesystem::path> scenePath_{};

    void handleSave() { editorWorkspace_.saveScene(scenePath_->string().c_str()); }

    void handleLoad() { editorWorkspace_.loadScene(scenePath_->string().c_str()); }

    void openSaveDialog() {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "json"}};
        if (NFD::SaveDialog(outPath, filters, 1) == NFD_OKAY) {
            scenePath_ = outPath;
            handleSave();
            NFD::FreePath(outPath);
        }
    }

    void openLoadDialog() {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "json"}};
        if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
            scenePath_ = outPath;
            handleLoad();
            NFD::FreePath(outPath);
        }
    }
};
