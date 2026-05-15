#include "EditorSelection.hpp"
#include <algorithm>

void EditorSelection::addToSelection(EntityHandle entity) {
    auto it = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
    if (it == selectedEntities_.end()) {
        selectedEntities_.push_back(entity);
    }
}

void EditorSelection::addToSelection(AssetHandle asset) {
    auto it = std::find(selectedAssets_.begin(), selectedAssets_.end(), asset);
    if (it == selectedAssets_.end()) {
        selectedAssets_.push_back(asset);
    }
}

void EditorSelection::removeFromSelection(EntityHandle entity) {
    auto it = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
    if (it != selectedEntities_.end()) {
        selectedEntities_.erase(it);
    }
}

void EditorSelection::removeFromSelection(AssetHandle asset) {
    auto it = std::find(selectedAssets_.begin(), selectedAssets_.end(), asset);
    if (it != selectedAssets_.end()) {
        selectedAssets_.erase(it);
    }
}

void EditorSelection::clearSelection() {
    selectedEntities_.clear();
    selectedAssets_.clear();
}

bool EditorSelection::isEntitySelected(EntityHandle entity) const {
    return std::find(selectedEntities_.begin(), selectedEntities_.end(), entity) != selectedEntities_.end();
}

bool EditorSelection::isAssetSelected(const AssetHandle& asset) const {
    return std::find(selectedAssets_.begin(), selectedAssets_.end(), asset) != selectedAssets_.end();
}
