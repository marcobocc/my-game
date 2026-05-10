#include "SceneMutations.hpp"
#include "../ObjectSelection.hpp"

void SceneMutations::clearSelectionIfSelected(EntityHandle deletedId) {
    auto selected = objectSelection_.getSelectedEntityId();
    if (selected && *selected == deletedId) {
        objectSelection_.clearSelection();
    }
}
