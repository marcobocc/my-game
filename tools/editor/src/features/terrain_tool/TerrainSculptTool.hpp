#pragma once
#include <optional>
#include <string>
#include "../../../../../runtime/src/graphics/assets/Mesh.hpp"
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

enum class TerrainBrushOp { RAISE, LOWER, SMOOTH, FLATTEN };

/*
    TerrainSculptTool

    Editor-mode-only tool that lets the user raise/lower/smooth/flatten a selected
    terrain mesh asset (an ordinary grid mesh, see terrain.md) by dragging the mouse
    over it in the viewport. Vertices only ever move along Y, so the mesh's XZ layout
    stays a regular grid and its resolution can be inferred as sqrt(vertexCount).

    A stroke (mouse-down to mouse-up) accumulates edits on a local working copy of the
    mesh, live-uploads it to the GPU each frame via VulkanBackend's resources manager,
    and only commits the change to the asset (one UndoHistory entry) on mouse-up.
*/
class TerrainSculptTool {
public:
    TerrainSculptTool(World& world,
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

    TerrainBrushOp brushOp() const { return brushOp_; }
    void setBrushOp(TerrainBrushOp op) { brushOp_ = op; }

    float radius() const { return radius_; }
    void setRadius(float r) { radius_ = r; }
    float strength() const { return strength_; }
    void setStrength(float s) { strength_ = s; }

    // Called once per frame (editor mode only). Draws the brush cursor into `debugDraw`
    // and, while the left mouse button is held, applies the active brush op.
    void update(DebugDraw& debugDraw);

private:
    struct TargetMesh {
        EntityHandle entity;
        std::string meshName;
        glm::mat4 modelMatrix;
        glm::mat4 invModelMatrix;
    };

    struct Stroke {
        std::string meshName;
        EntityHandle entity;
        Mesh workingMesh;
    };

    std::optional<TargetMesh> findTarget() const;
    // Terrains have no dedicated marker (see terrain.md) — a mesh is only sculptable if its
    // topology matches AssetPrefabs::terrainMesh() output: a square grid of vertices with the matching
    // triangle count. This excludes ordinary models/primitives from being sculpted by accident.
    static bool isGridMesh(const Mesh& mesh);
    std::optional<glm::vec3> raycastLocal(const TargetMesh& target, glm::vec3& outNormal) const;
    void applyBrush(Mesh& mesh, const glm::vec3& localHit, float deltaTime);
    void beginStroke(const TargetMesh& target);
    void endStroke();
    static void recomputeNormals(Mesh& mesh);
    void drawWireframe(DebugDraw& debugDraw, const TargetMesh& target, const Mesh& mesh) const;

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
    TerrainBrushOp brushOp_ = TerrainBrushOp::RAISE;
    float radius_ = 3.0f;
    float strength_ = 2.0f;

    bool wasLeftDown_ = false;
    std::optional<Stroke> stroke_;
};
