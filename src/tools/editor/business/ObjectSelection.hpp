#pragma once
#include <optional>
#include <string>
#include <variant>
#include "modules/scene/EntityManager.hpp"

using AssetHandle = std::string;

class ObjectSelection {
public:
    using SelectedElement = std::variant<EntityHandle, AssetHandle>;

    void selectObject(EntityHandle entity);
    void selectAsset(AssetHandle asset);
    void clearSelection();

    const std::optional<SelectedElement>& getSelectedElement() const { return selectedElement_; }
    bool isElementGameObject() const;
    bool isElementAsset() const;

    std::optional<EntityHandle> getSelectedEntityId() const;
    std::optional<AssetHandle> getSelectedAssetId() const;

private:
    std::optional<SelectedElement> selectedElement_;
};
