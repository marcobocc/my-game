#pragma once
#include <optional>
#include "modules/scene/EntityManager.hpp"

class ObjectSelection {
public:
    void selectObject(EntityHandle entity);
    void clearSelection();

    std::optional<EntityHandle> getSelectedEntityId() const { return selectedEntityId_; }

private:
    std::optional<EntityHandle> selectedEntityId_;
};
