#pragma once

struct RendererSettings;

class EditorSettings {
public:
    explicit EditorSettings(RendererSettings& rendererSettings);

    void toggleLighting();
    bool isLightingEnabled() const { return lightingEnabled_; }

    void enableGrid();
    void disableGrid();
    void toggleGrid();
    bool isGridEnabled() const { return gridEnabled_; }

    float getGridScale() const { return gridScale_; }
    void increaseScale();
    void decreaseScale();
    void resetScale();

    bool isGridSnappingEnabled() const { return gridSnappingEnabled_; }
    void enableSnapping();
    void disableSnapping();
    void toggleSnapping();

private:
    RendererSettings& rendererSettings_;

    float gridScale_ = 1.0f;
    bool gridEnabled_ = true;
    bool gridSnappingEnabled_ = true;
    bool lightingEnabled_ = true;

    static constexpr float GRID_RESCALE_FACTOR = 1.5f;
    static constexpr float GRID_SCALE_UPPER_BOUND = 3.375f;
    static constexpr float GRID_SCALE_LOWER_BOUND = 1.0f / 29274466.0f;
};
