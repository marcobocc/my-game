#include "ObjectSelection.hpp"

void ObjectSelection::selectObject(EntityHandle entity) {
    if (selectedEntityId_ && *selectedEntityId_ == entity)
        selectedEntityId_.reset();
    else
        selectedEntityId_.emplace(entity);
}

void ObjectSelection::clearSelection() { selectedEntityId_.reset(); }
