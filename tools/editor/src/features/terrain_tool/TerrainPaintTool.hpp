#pragma once
#include <optional>
#include <string>
#include "../../../../../runtime/src/graphics/assets/Material.hpp"
#include "../../../../../runtime/src/graphics/assets/Texture.hpp"
#include "../../services/EditorSelection.hpp"
#include "../../services/UndoHistory.hpp"
#include "../../services/common_editing/AssetStore.hpp"
#include "../../services/common_editing/RuntimeScene.hpp"
#include "../scene_viewport/editor_camera/EditorCamera.hpp"
#include "core/GameWindow.hpp"
#include "core/scene/World.hpp"
#include "input/InputSystem.hpp"

class VulkanBackend;
class DebugDraw;

/*
    TerrainPaintTool

    Editor-mode-only tool that lets the user paint splat weights (which layer texture shows
    where) onto a terrain entity's splatmap texture asset, by dragging the mouse over it in
    the viewport. Reuses TerrainSculptTool's raycast/stroke/undo pattern, but writes into a
    Texture asset's RGBA texel data instead of a Mesh's vertex positions.

    The splatmap texel to paint is found by barycentrically interpolating the mesh's actual
    UVs at the raycast hit — the same UVs the terrain-blend shader samples the splatmap with.
    (Mapping the hit's local XZ linearly to UV is not safe: the mesh asset round-trips through
    an OBJ import, which does not preserve the generator's UV orientation.)
*/
class TerrainPaintTool {
public:
    TerrainPaintTool(World& world,
                     RuntimeScene& scene,
                     EditorSelection& selection,
                     AssetStore& assetStore,
                     EditorCamera& camera,
                     InputSystem& inputSystem,
                     GameWindow& window,
                     VulkanBackend& backend,
                     UndoHistory& undoHistory) :
        world_(world),
        scene_(scene),
        selection_(selection),
        assetStore_(assetStore),
        camera_(camera),
        inputSystem_(inputSystem),
        window_(window),
        backend_(backend),
        undoHistory_(undoHistory) {}

    bool isActive() const { return active_; }
    void setActive(bool active) {
        active_ = active;
        if (!active_ && stroke_) endStroke();
    }

    int activeLayer() const { return activeLayer_; }
    void setActiveLayer(int layer) { activeLayer_ = layer; }

    float radius() const { return radius_; }
    void setRadius(float r) { radius_ = r; }
    float strength() const { return strength_; }
    void setStrength(float s) { strength_ = s; }

    // Called once per frame (editor mode only). Draws the brush cursor into `debugDraw`
    // and, while the left mouse button is held, paints the active layer's weight.
    void update(DebugDraw& debugDraw);

private:
    struct TargetTerrain {
        EntityHandle entity;
        std::string meshName;
        std::string splatMapTexture;
        glm::mat4 modelMatrix;
        glm::mat4 invModelMatrix;
        float minX, maxX, minZ, maxZ;
    };

    struct Stroke {
        std::string splatMapName;
        Texture workingTexture;
    };

    struct LocalHit {
        glm::vec3 position; // mesh local space
        glm::vec2 uv; // interpolated mesh UV at the hit
    };

    std::optional<TargetTerrain> findTarget() const;
    std::optional<LocalHit> raycastLocal(const TargetTerrain& target) const;
    void applyBrush(Texture& tex, const TargetTerrain& target, const LocalHit& hit, float deltaTime);
    void beginStroke(const TargetTerrain& target);
    void endStroke();

    World& world_;
    RuntimeScene& scene_;
    EditorSelection& selection_;
    AssetStore& assetStore_;
    EditorCamera& camera_;
    InputSystem& inputSystem_;
    GameWindow& window_;
    VulkanBackend& backend_;
    UndoHistory& undoHistory_;

    bool active_ = false;
    int activeLayer_ = 0; // 0..3, indexes weights.r/g/b/a
    float radius_ = 3.0f;
    float strength_ = 3.0f;

    bool wasLeftDown_ = false;
    std::optional<Stroke> stroke_;
};
