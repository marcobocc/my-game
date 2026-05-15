#pragma once
#include <string>
#include <vector>
#include "modules/scene/EntityManager.hpp"

using AssetHandle = std::string;

class EditorSelection {
public:
    void addToSelection(EntityHandle entity);
    void addToSelection(AssetHandle asset);
    void removeFromSelection(EntityHandle entity);
    void removeFromSelection(AssetHandle asset);
    void clearSelection();

    bool isEntitySelected(EntityHandle entity) const;
    bool isAssetSelected(const AssetHandle& asset) const;

    const std::vector<EntityHandle>& getSelectedEntityIds() const { return selectedEntities_; }
    const std::vector<AssetHandle>& getSelectedAssetIds() const { return selectedAssets_; }

private:
    std::vector<EntityHandle> selectedEntities_;
    std::vector<AssetHandle> selectedAssets_;
};
