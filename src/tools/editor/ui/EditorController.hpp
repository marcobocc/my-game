#pragma once
#include <fstream>
#include <nfd.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include "EditorMenuBar.hpp"
#include "HierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "core/GameEngine.hpp"
#include "core/scene/SceneSerializer.hpp"
#include "utils/JsonUtils.hpp"

class EditorController {
public:
    std::optional<std::string> selectedObjectId{};
    std::optional<std::filesystem::path> scenePath{};
    std::string cameraId{};

    explicit EditorController(GameEngine& engine) : engine_(engine) {
        menuBar_ = engine_.getUserInterface().emplace<EditorMenuBar>();
        menuBar_->onSave = [this] { saveScene(*scenePath); };
        menuBar_->onSaveAs = [this] { openSaveDialog(); };
        menuBar_->onOpen = [this] { openLoadDialog(); };
        engine_.getUserInterface().emplace<HierarchyPanel>(&selectedObjectId, &engine_.getScene());
        engine_.getUserInterface().emplace<InspectorPanel>(
                &selectedObjectId, &engine_.getScene(), &engine_.getAssetManager());
    }

    void saveScene(const std::filesystem::path& path) {
        std::ofstream f(path);
        if (!f) return;
        f << SceneSerializer::serializeScene(engine_.getScene()).dump(4);
        scenePath = path;
        menuBar_->sceneName = path.filename().string();
    }

    void loadScene(const std::filesystem::path& path) {
        try {
            SceneSerializer::deserializeScene(JsonUtils::loadJson(path), engine_.getScene());
            scenePath = path;
            menuBar_->sceneName = path.filename().string();
            selectedObjectId = std::nullopt;
        } catch (const std::exception&) {
        }
    }

private:
    GameEngine& engine_;
    EditorMenuBar* menuBar_ = nullptr;

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
