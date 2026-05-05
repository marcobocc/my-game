#pragma once
#include <optional>
#include <string>

class ObjectSelection {
public:
    void selectObject(const std::string& objectId);
    void clearSelection();

    std::optional<std::string> getSelectedObjectId() const { return selectedObjectId_; }

private:
    std::optional<std::string> selectedObjectId_;
};
