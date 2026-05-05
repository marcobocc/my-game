#pragma once
#include <fstream>
#include <functional>
#include <nfd.hpp>
#include <optional>
#include <string>
#include "../../../engine/systems/assets/AssetManager.hpp"
#include "../../../engine/systems/scene/SceneSerializer.hpp"
#include "../../../engine/systems/ui/UserInterface.hpp"
#include "../view/imgui_widgets/containers/EditorMenuBar.hpp"
#include "../view/imgui_widgets/containers/HierarchyPanel.hpp"
#include "../view/imgui_widgets/containers/InspectorPanel.hpp"
#include "SceneMutationsController.hpp"
#include "utils/JsonUtils.hpp"


class EditorUIController {
public:
    std::optional<std::filesystem::path> scenePath{};
    SceneMutationsController mutations;

    EditorUIController(GameEngine& engine, UserInterface& ui, EditorState& editorState, AssetManager& assetManager) :
        mutations(editorState.getScene(), engine),
        userInterface_(ui),
        editorState_(editorState),
        assetManager_(assetManager) {
        setupUI();
    }

    void saveScene(const std::filesystem::path& path) {
        std::ofstream f(path);
        if (!f) return;
        f << SceneSerializer::serializeScene(editorState_.getScene()).dump(4);
        scenePath = path;
        if (menuBar_) menuBar_->sceneName = path.filename().string();
    }

    void loadScene(const std::filesystem::path& path) {
        try {
            SceneSerializer::deserializeScene(JsonUtils::loadJson(path), editorState_.getScene());
            scenePath = path;
            if (menuBar_) menuBar_->sceneName = path.filename().string();
            editorState_.setSelectedObject(std::nullopt);
        } catch (const std::exception&) {
        }
    }

private:
    UserInterface& userInterface_;
    EditorState& editorState_;
    AssetManager& assetManager_;
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
        userInterface_.emplace<HierarchyPanel>(editorState_, editorState_.getScene(), assetManager_, mutations);

        auto aabbToggle = [this](const std::string& objectId, bool show) {
            if (show)
                editorState_.enableAABB(objectId);
            else
                editorState_.disableAABB(objectId);
        };

        auto sphereToggle = [this](const std::string& objectId, bool show) {
            if (show)
                editorState_.enableBoundingSphere(objectId);
            else
                editorState_.disableBoundingSphere(objectId);
        };

        userInterface_.emplace<InspectorPanel>(
                editorState_, editorState_.getScene(), assetManager_, mutations, aabbToggle, sphereToggle);
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
