#pragma once
#include <functional>
#include <vector>

// Macro editor modes: which panel set, dock layout, and viewport input routing
// is active. See editor-changes.md.
enum class EditorMode {
    Selection, // hierarchy / inspector / gizmos — the default editing layout
    Terrain, // terrain sculpt/paint tools own the viewport input
};

class EditorModeService {
public:
    EditorMode mode() const { return mode_; }

    void setMode(EditorMode mode) {
        if (mode == mode_) return;
        mode_ = mode;
        for (const auto& cb: subscribers_)
            cb(mode);
    }

    void subscribe(std::function<void(EditorMode)> onChange) { subscribers_.push_back(std::move(onChange)); }

private:
    EditorMode mode_ = EditorMode::Selection;
    std::vector<std::function<void(EditorMode)>> subscribers_;
};
