#include "ObjectSelection.hpp"

void ObjectSelection::selectObject(EntityHandle entity) {
    if (isElementGameObject() && std::get<EntityHandle>(*selectedElement_) == entity) {
        selectedElement_.reset();
    } else {
        selectedElement_.emplace(entity);
    }
}

void ObjectSelection::selectAsset(AssetHandle asset) {
    if (isElementAsset() && std::get<AssetHandle>(*selectedElement_) == asset) {
        selectedElement_.reset();
    } else {
        selectedElement_.emplace(asset);
    }
}

void ObjectSelection::clearSelection() { selectedElement_.reset(); }

bool ObjectSelection::isElementGameObject() const {
    return selectedElement_ && std::holds_alternative<EntityHandle>(*selectedElement_);
}

bool ObjectSelection::isElementAsset() const {
    return selectedElement_ && std::holds_alternative<AssetHandle>(*selectedElement_);
}

bool ObjectSelection::isElementMaterial() const {
    return isElementAsset() && std::get<AssetHandle>(*selectedElement_).ends_with(".mat");
}

std::optional<EntityHandle> ObjectSelection::getSelectedEntityId() const {
    if (isElementGameObject()) {
        return std::get<EntityHandle>(*selectedElement_);
    }
    return std::nullopt;
}

std::optional<AssetHandle> ObjectSelection::getSelectedAssetId() const {
    if (isElementAsset()) {
        return std::get<AssetHandle>(*selectedElement_);
    }
    return std::nullopt;
}
