#pragma once
#include <fstream>
#include <nfd.hpp>
#include <optional>
#include <string>
#include <unordered_set>
#include "EditorMenuBar.hpp"
#include "GameEngine.hpp"
#include "HierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "utils/JsonUtils.hpp"

class EditorController {
public:
    std::optional<std::string> selectedObjectId{};
    std::optional<std::filesystem::path> scenePath{};
    std::string cameraId{};
    std::unordered_set<std::string> aabbGizmoEnabled{};

    explicit EditorController(GameEngine& engine) : engine_(engine) {
        menuBar_ = engine_.emplaceWidget<EditorMenuBar>();
        menuBar_->onSave = [this] { saveScene(*scenePath); };
        menuBar_->onSaveAs = [this] { openSaveDialog(); };
        menuBar_->onOpen = [this] { openLoadDialog(); };
        engine_.emplaceWidget<HierarchyPanel>(&selectedObjectId, engine_);
        engine_.emplaceWidget<InspectorPanel>(&selectedObjectId, engine_, aabbGizmoEnabled);
    }

    void saveScene(const std::filesystem::path& path) {
        std::ofstream f(path);
        if (!f) return;
        f << engine_.serializeScene().dump(4);
        scenePath = path;
        menuBar_->sceneName = path.filename().string();
    }

    void loadScene(const std::filesystem::path& path) {
        try {
            engine_.deserializeScene(JsonUtils::loadJson(path));
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
