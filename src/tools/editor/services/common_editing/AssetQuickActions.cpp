#include "AssetQuickActions.hpp"
#include "details/AssetPrefabs.hpp"

AssetHandle AssetQuickActions::createMaterial(const std::string& folder) {
    std::string materialName = generateMaterialName(folder);
    assetStore_.createAssetFile<Material>(AssetPrefabs::solidColor(materialName));
    editorSelection_.addToSelection(materialName);
    return materialName;
}

void AssetQuickActions::deleteSelection() {
    const auto& assetNames = editorSelection_.getSelectedAssetIds();
    if (assetNames.empty()) return;
    const std::string groupId = UndoHistory::randomGroupId("Delete Asset Selection");
    for (const auto& name: assetNames) {
        assetStore_.destroyAssetFile(name, groupId);
    }
    editorSelection_.clearSelection();
}

std::string AssetQuickActions::generateMaterialName(const std::string& folder) const {
    auto availableMaterials = assetStore_.list(".mat");
    std::string prefix = folder.empty() ? "" : folder + "/";
    int counter = 1;
    std::string materialName = prefix + "New Material (" + std::to_string(counter) + ").mat";
    while (std::ranges::find(availableMaterials, materialName) != availableMaterials.end()) {
        counter++;
        materialName = prefix + "New Material (" + std::to_string(counter) + ").mat";
    }
    return materialName;
}
