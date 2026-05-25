#include "UndoHistory.hpp"

void UndoHistory::push(Action undo, Action redo, const std::string& description, const std::string& groupId) {
    stack_.erase(stack_.begin() + position_, stack_.end());
    Entry e{std::move(undo), std::move(redo), description.empty() ? "Unknown" : description, groupId};
    stack_.push_back(e);
    position_ = static_cast<int>(stack_.size());
    LOG4CXX_INFO(LOGGER, "Pushed action: " + entryToString(e));
}

void UndoHistory::undo() {
    if (position_ <= 0) return;
    const auto targetGroup = stack_[position_ - 1].groupId;
    std::vector<int> indices;
    int scan = position_ - 1;
    while (scan >= 0) {
        if (!targetGroup.empty() && stack_[scan].groupId != targetGroup) break;
        indices.push_back(scan);
        if (targetGroup.empty()) break;
        --scan;
    }
    LOG4CXX_INFO(LOGGER, "Undoing: " + groupToString(indices, stack_));
    for (int idx: indices) {
        position_ = idx;
        stack_[idx].undo();
    }
}

void UndoHistory::redo() {
    if (position_ >= static_cast<int>(stack_.size())) return;
    const auto targetGroup = stack_[position_].groupId;
    std::vector<int> indices;
    int scan = position_;
    while (scan < static_cast<int>(stack_.size())) {
        if (!targetGroup.empty() && stack_[scan].groupId != targetGroup) break;
        indices.push_back(scan);
        if (targetGroup.empty()) break;
        ++scan;
    }
    LOG4CXX_INFO(LOGGER, "Redoing: " + groupToString(indices, stack_));
    for (int idx: indices) {
        stack_[idx].redo();
        position_ = idx + 1;
    }
}

void UndoHistory::clear() {
    stack_.clear();
    position_ = 0;
    LOG4CXX_INFO(LOGGER, "Clear undo history");
}

std::string UndoHistory::entryToString(const Entry& e) {
    return e.groupId.empty() ? e.description : "[" + e.groupId + "] " + e.description;
}

std::string UndoHistory::groupToString(const std::vector<int>& indices, const std::vector<Entry>& stack) {
    std::string result = "{";
    bool first = true;
    constexpr std::size_t truncThreshold = 2;
    std::string last;
    std::size_t runCount = 0;
    bool truncated = false;
    auto flush = [&](const std::string& s, std::size_t count) {
        if (count == 0) return;
        std::size_t shown = std::min(count, truncThreshold);
        for (std::size_t i = 0; i < shown; ++i) {
            if (!first) result += ", ";
            result += s;
            first = false;
        }
        if (count > truncThreshold) truncated = true;
    };
    for (int idx: indices) {
        std::string s = entryToString(stack[idx]);
        if (s == last) {
            ++runCount;
        } else {
            flush(last, runCount);
            last = s;
            runCount = 1;
        }
    }
    flush(last, runCount);
    if (truncated) result += ", ...";
    return result + "}";
}
