#pragma once
#include <optional>
#include <string>
#include <vector>
#include "modules/scene/EntityStore.hpp"

using AssetHandle = std::string;

enum class SelectionType { NONE, OBJECTS, ASSETS };

class EditorSelection {
public:
    void addToSelection(EntityHandle entity, bool extend = false);
    void addToSelection(AssetHandle asset, bool extend = false);
    void removeFromSelection(EntityHandle entity);
    void removeFromSelection(AssetHandle asset);
    void clearSelection();

    bool isEntitySelected(EntityHandle entity) const;
    bool isAssetSelected(const AssetHandle& asset) const;

    SelectionType getSelectionType() const { return type_; }
    const std::vector<EntityHandle>& getSelectedEntityIds() const { return selectedEntities_; }
    const std::vector<AssetHandle>& getSelectedAssetIds() const { return selectedAssets_; }

    void setInspectedAsset(const AssetHandle& asset) { inspectedAsset_ = asset; }
    void clearInspectedAsset() { inspectedAsset_.reset(); }
    const std::optional<AssetHandle>& getInspectedAsset() const { return inspectedAsset_; }

private:
    SelectionType type_ = SelectionType::NONE;
    std::vector<EntityHandle> selectedEntities_;
    std::vector<AssetHandle> selectedAssets_;
    std::optional<AssetHandle> inspectedAsset_;
};
