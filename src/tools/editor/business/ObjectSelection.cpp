#include "ObjectSelection.hpp"

void ObjectSelection::selectObject(const std::string& objectId) {
    if (selectedObjectId_ == objectId) {
        selectedObjectId_ = {};
    } else {
        selectedObjectId_ = objectId;
    }
}

void ObjectSelection::clearSelection() { selectedObjectId_ = {}; }
