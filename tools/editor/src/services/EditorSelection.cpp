#include "EditorSelection.hpp"
#include <algorithm>

void EditorSelection::addToSelection(EntityHandle entity, bool extend) {
    if (type_ != SelectionType::OBJECTS) {
        selectedEntities_.clear();
        selectedAssets_.clear();
        inspectedAsset_.reset();
        type_ = SelectionType::OBJECTS;
    } else if (!extend) {
        selectedEntities_.clear();
    }
    auto it = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
    if (it == selectedEntities_.end()) {
        selectedEntities_.push_back(entity);
    }
}

void EditorSelection::addToSelection(AssetHandle asset, bool extend) {
    if (type_ != SelectionType::ASSETS) {
        selectedEntities_.clear();
        selectedAssets_.clear();
        type_ = SelectionType::ASSETS;
    } else if (!extend) {
        selectedAssets_.clear();
    }
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
    if (selectedEntities_.empty() && selectedAssets_.empty()) type_ = SelectionType::NONE;
}

void EditorSelection::removeFromSelection(AssetHandle asset) {
    auto it = std::find(selectedAssets_.begin(), selectedAssets_.end(), asset);
    if (it != selectedAssets_.end()) {
        selectedAssets_.erase(it);
    }
    if (selectedEntities_.empty() && selectedAssets_.empty()) type_ = SelectionType::NONE;
}

void EditorSelection::clearSelection() {
    selectedEntities_.clear();
    selectedAssets_.clear();
    type_ = SelectionType::NONE;
}

bool EditorSelection::isEntitySelected(EntityHandle entity) const {
    return std::find(selectedEntities_.begin(), selectedEntities_.end(), entity) != selectedEntities_.end();
}

bool EditorSelection::isAssetSelected(const AssetHandle& asset) const {
    return std::find(selectedAssets_.begin(), selectedAssets_.end(), asset) != selectedAssets_.end();
}
