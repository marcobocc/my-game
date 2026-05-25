#pragma once
#include <string>
#include "../../services/EditorSelection.hpp"
#include "../UndoHistory.hpp"
#include "AssetStore.hpp"

class AssetQuickActions {
public:
    AssetQuickActions(AssetStore& assetStore, EditorSelection& editorSelection, UndoHistory& undoHistory) :
        assetStore_(assetStore),
        editorSelection_(editorSelection),
        undoHistory_(undoHistory) {}

    AssetHandle createMaterial(const std::string& folder = "");

    void deleteSelection();

private:
    AssetStore& assetStore_;
    EditorSelection& editorSelection_;
    UndoHistory& undoHistory_;

    std::string generateMaterialName(const std::string& folder) const;
};
