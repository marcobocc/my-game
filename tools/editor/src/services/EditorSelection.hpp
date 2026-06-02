#pragma once
#include <optional>
#include <string>
#include <vector>
#include "modules/scene/EntityHandle.hpp"

using AssetHandle = std::string;

class EditorSelection {
public:
    void addToSelection(EntityHandle entity, bool extend = false);
    void addToSelection(AssetHandle asset, bool extend = false);
    void removeFromSelection(EntityHandle entity);
    void removeFromSelection(AssetHandle asset);
    void clearSelection();

    bool isEntitySelected(EntityHandle entity) const;
    bool isAssetSelected(const AssetHandle& asset) const;

    const std::vector<EntityHandle>& getSelectedEntityIds() const { return selectedEntities_; }
    const std::vector<AssetHandle>& getSelectedAssetIds() const { return selectedAssets_; }

    void setInspectedEntity(EntityHandle entity) { inspectedEntity_ = entity; }
    void clearInspectedEntity() { inspectedEntity_.reset(); }
    const std::optional<EntityHandle>& getInspectedEntity() const { return inspectedEntity_; }

    void setInspectedAsset(const AssetHandle& asset) { inspectedAsset_ = asset; }
    void clearInspectedAsset() { inspectedAsset_.reset(); }
    const std::optional<AssetHandle>& getInspectedAsset() const { return inspectedAsset_; }

private:
    std::vector<EntityHandle> selectedEntities_;
    std::vector<AssetHandle> selectedAssets_;
    std::optional<EntityHandle> inspectedEntity_;
    std::optional<AssetHandle> inspectedAsset_;
};
