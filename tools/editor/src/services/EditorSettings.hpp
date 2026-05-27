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

    bool isLocalTransformEnabled() const { return localTransform_; }
    void enableLocalTransform();
    void disableLocalTransform();
    void toggleLocalTransform();

private:
    RendererSettings& rendererSettings_;

    float gridScale_ = 1.0f;
    bool gridEnabled_ = true;
    bool gridSnappingEnabled_ = false;
    bool lightingEnabled_ = true;
    bool localTransform_ = true;

    static constexpr float GRID_RESCALE_FACTOR = 1.5f;
    static constexpr float GRID_SCALE_UPPER_BOUND = 3.375f;
    static constexpr float GRID_SCALE_LOWER_BOUND = 1.0f / 29274466.0f;
};
