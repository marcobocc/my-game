#pragma once
#include <functional>
#include <vector>

class UndoHistoryController {
public:
    using Action = std::function<void()>;

    void push(Action undo, Action redo) {
        stack_.erase(stack_.begin() + position_, stack_.end());
        stack_.push_back({std::move(undo), std::move(redo)});
        position_ = static_cast<int>(stack_.size());
    }

    void undo() {
        if (position_ <= 0) return;
        --position_;
        stack_[position_].undo();
    }

    void redo() {
        if (position_ >= static_cast<int>(stack_.size())) return;
        stack_[position_].redo();
        ++position_;
    }

private:
    struct Entry {
        Action undo;
        Action redo;
    };

    std::vector<Entry> stack_;
    int position_ = 0;
};
