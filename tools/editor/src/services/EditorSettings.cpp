#include "EditorSettings.hpp"
#include <algorithm>
#include "graphics/RendererSettings.hpp"

EditorSettings::EditorSettings(RendererSettings& rendererSettings) : rendererSettings_(rendererSettings) {}

void EditorSettings::toggleLighting() {
    lightingEnabled_ = !lightingEnabled_;
    rendererSettings_.enableLighting = lightingEnabled_;
}

void EditorSettings::enableGrid() {
    gridEnabled_ = true;
    rendererSettings_.enableGrid = true;
}

void EditorSettings::disableGrid() {
    gridEnabled_ = false;
    rendererSettings_.enableGrid = false;
}

void EditorSettings::toggleGrid() {
    gridEnabled_ = !gridEnabled_;
    rendererSettings_.enableGrid = gridEnabled_;
}

void EditorSettings::increaseScale() {
    float newScale = gridScale_ * GRID_RESCALE_FACTOR;
    gridScale_ = std::clamp(newScale, GRID_SCALE_LOWER_BOUND, GRID_SCALE_UPPER_BOUND);
}

void EditorSettings::decreaseScale() {
    float newScale = gridScale_ / GRID_RESCALE_FACTOR;
    gridScale_ = std::clamp(newScale, GRID_SCALE_LOWER_BOUND, GRID_SCALE_UPPER_BOUND);
}

void EditorSettings::resetScale() { gridScale_ = 1.0f; }

void EditorSettings::enableSnapping() { gridSnappingEnabled_ = true; }

void EditorSettings::disableSnapping() { gridSnappingEnabled_ = false; }

void EditorSettings::toggleSnapping() { gridSnappingEnabled_ = !gridSnappingEnabled_; }

void EditorSettings::enableLocalTransform() { localTransform_ = true; }

void EditorSettings::disableLocalTransform() { localTransform_ = false; }

void EditorSettings::toggleLocalTransform() { localTransform_ = !localTransform_; }
