#pragma once
#include <fstream>
#include <nfd.hpp>
#include <optional>
#include <string>
#include "../../../engine/GameEngine.hpp"
#include "../../../engine/systems/ui/UserInterface.hpp"
#include "../state/EditorState.hpp"
#include "../view/imgui_widgets/containers/EditorMenuBar.hpp"
#include "../view/imgui_widgets/containers/HierarchyPanel.hpp"
#include "../view/imgui_widgets/containers/InspectorPanel.hpp"
#include "SceneMutationsController.hpp"
#include "utils/JsonUtils.hpp"


class EditorUIController {
public:
    std::optional<std::filesystem::path> scenePath{};
    SceneMutationsController mutations;

    EditorUIController(GameEngine& engine, UserInterface& ui, EditorState& editorState) :
        mutations(engine),
        engine_(engine),
        userInterface_(ui),
        editorState_(editorState) {
        setupUI();
    }

    void saveScene(const std::filesystem::path& path) {
        std::ofstream f(path);
        if (!f) return;
        f << engine_.serializeScene().dump(4);
        scenePath = path;
        if (menuBar_) menuBar_->sceneName = path.filename().string();
    }

    void loadScene(const std::filesystem::path& path) {
        try {
            engine_.deserializeScene(JsonUtils::loadJson(path));
            scenePath = path;
            if (menuBar_) menuBar_->sceneName = path.filename().string();
            editorState_.setSelectedObject(std::nullopt);
        } catch (const std::exception&) {
        }
    }

private:
    GameEngine& engine_;
    UserInterface& userInterface_;
    EditorState& editorState_;
    EditorMenuBar* menuBar_ = nullptr;

    void setupUI() {
        menuBar_ = userInterface_.emplace<EditorMenuBar>();
        menuBar_->onSave = [this] {
            if (scenePath) saveScene(*scenePath);
        };
        menuBar_->onSaveAs = [this] { openSaveDialog(); };
        menuBar_->onOpen = [this] { openLoadDialog(); };
        menuBar_->onUndo = [this] { mutations.undoHistory().undo(); };
        menuBar_->onRedo = [this] { mutations.undoHistory().redo(); };
        userInterface_.emplace<HierarchyPanel>(&editorState_.getSelectedObjectRef(), engine_, mutations);
        userInterface_.emplace<InspectorPanel>(&editorState_.getSelectedObjectRef(), engine_, mutations);
    }

    void openSaveDialog() {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "json"}};
        if (NFD::SaveDialog(outPath, filters, 1) == NFD_OKAY) {
            saveScene(outPath);
            NFD::FreePath(outPath);
        }
    }

    void openLoadDialog() {
        NFD::Guard nfdGuard;
        nfdnchar_t* outPath = nullptr;
        nfdfilteritem_t filters[] = {{"Scene", "json"}};
        if (NFD::OpenDialog(outPath, filters, 1) == NFD_OKAY) {
            loadScene(outPath);
            NFD::FreePath(outPath);
        }
    }
};
