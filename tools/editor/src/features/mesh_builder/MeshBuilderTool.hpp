#pragma once
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include "../../../../../runtime/src/graphics/assets/Mesh.hpp"
#include "../../services/EditorSelection.hpp"
#include "../../services/UndoHistory.hpp"
#include "../../services/common_editing/AssetStore.hpp"
#include "../../services/common_editing/RuntimeScene.hpp"
#include "../scene_viewport/editor_camera/EditorCamera.hpp"
#include "../scene_viewport/gizmos/gizmos.hpp"
#include "core/GameWindow.hpp"
#include "core/scene/World.hpp"
#include "input/InputSystem.hpp"

class VulkanBackend;
class DebugDraw;
class ObjectTransformHandle;
class EditorRenderer;

enum class MeshBuilderSelectMode { Vertex, Edge, Face };

/*
    MeshBuilderTool

    Editor-mode-only tool that lets the user select a vertex or an edge of a selected
    mesh asset and drag it around the viewport. Vertices that share the same position
    (e.g. across a UV seam) are grouped into a single "logical vertex" so edits never
    tear the mesh apart.

    Follows the same stroke pattern as TerrainSculptTool: a mouse-down to mouse-up drag
    edits a local working copy of the mesh, live-uploads it to the GPU every frame, and
    only commits the change to the asset (one UndoHistory entry) on mouse-up.
*/
class MeshBuilderTool {
public:
    MeshBuilderTool(World& world,
                    RuntimeScene& scene,
                    EditorSelection& selection,
                    AssetStore& assetStore,
                    EditorCamera& camera,
                    InputSystem& inputSystem,
                    GameWindow& window,
                    VulkanBackend& backend,
                    UndoHistory& undoHistory,
                    ObjectTransformHandle& objectTransformHandle,
                    EditorRenderer& editorRenderer) :
        world_(world),
        scene_(scene),
        selection_(selection),
        assetStore_(assetStore),
        camera_(camera),
        inputSystem_(inputSystem),
        window_(window),
        backend_(backend),
        undoHistory_(undoHistory),
        objectTransformHandle_(objectTransformHandle),
        editorRenderer_(editorRenderer) {}

    bool isActive() const { return active_; }
    void setActive(bool active) {
        active_ = active;
        if (!active_ && stroke_) endStroke();
    }

    MeshBuilderSelectMode selectMode() const { return selectMode_; }
    void setSelectMode(MeshBuilderSelectMode mode) { selectMode_ = mode; }

    // Called once per frame (editor mode only). Draws hover/selection highlights into
    // `debugDraw` and, while the left mouse button is held on a hovered element, drags it.
    void update(DebugDraw& debugDraw);

private:
    struct TargetMesh {
        EntityHandle entity;
        std::string meshName;
        glm::mat4 modelMatrix;
        glm::mat4 invModelMatrix;
    };

    // An edge between two logical vertices, identified by their canonical (lowest) vertex index.
    struct LogicalEdge {
        uint32_t a;
        uint32_t b;
    };

    // A group of coplanar, edge-connected triangles (e.g. a cube's quad face, baked as two
    // triangles). Faces are separated at hard edges (a normal discontinuity).
    struct LogicalFace {
        std::vector<uint32_t> triangleStarts; // index into mesh.getIndices(), one per triangle (multiples of 3)
        std::vector<uint32_t> vertices; // raw vertex indices referenced by this face's triangles
    };

    struct Stroke {
        std::string meshName;
        EntityHandle entity;
        Mesh workingMesh;
        std::vector<uint32_t> draggedIndices; // raw vertex indices being moved together
        std::vector<glm::vec3> initialLocalPositions; // draggedIndices' local positions at grab time
        glm::vec3 pivotWorld; // world-space grab point, used as the axis origin
        glm::vec3 axisDir; // world-space axis the drag is constrained to
        glm::vec3 planePoint; // world-space, fixed drag plane anchor (initial ray hit)
        glm::vec3 planeNormal; // world-space, fixed drag plane normal
        std::string mutationGroup; // shared with the fork-on-write step so undo reverts both together
    };

    std::optional<TargetMesh> findTarget() const;
    // Mesh assets (e.g. primitive prefabs like "Cube.mesh") are commonly shared by many
    // entities. Editing one must not affect the others, so the first edit of an entity's
    // mesh forks it into an entity-owned copy and repoints that entity's Renderer at it.
    // No-op if the mesh is already an entity-owned fork.
    void ensureUniqueMesh(TargetMesh& target, const std::string& mutationGroup);
    // Groups raw vertex indices that share (approximately) the same local position, so moving
    // one moves every vertex baked at that seam together.
    static std::vector<std::vector<uint32_t>> buildLogicalVertices(const Mesh& mesh);
    static std::vector<LogicalEdge> buildLogicalEdges(const Mesh& mesh,
                                                      const std::vector<std::vector<uint32_t>>& logicalVertices);
    // Groups triangles into flat faces: edge-connected triangles whose normals match.
    static std::vector<LogicalFace> buildLogicalFaces(const Mesh& mesh,
                                                      const std::vector<std::vector<uint32_t>>& logicalVertices);
    std::optional<glm::vec2> worldToScreen(const glm::vec3& world) const;
    std::optional<std::pair<glm::vec3, glm::vec3>> mouseRayWorld() const; // origin, direction
    static void recomputeNormals(Mesh& mesh);
    void beginStroke(const TargetMesh& target,
                     const std::vector<uint32_t>& draggedIndices,
                     const glm::vec3& pivotWorld,
                     const glm::vec3& axisDir,
                     const std::string& mutationGroup);
    void endStroke();
    void drawHighlights(DebugDraw& debugDraw,
                        const TargetMesh& target,
                        const Mesh& mesh,
                        const std::vector<std::vector<uint32_t>>& logicalVertices,
                        const std::vector<LogicalEdge>& logicalEdges,
                        const std::vector<LogicalFace>& logicalFaces) const;

    // Builds the same solid translation gizmo ObjectTransformHandle draws for entities, but
    // pivoted at the selected vertex/edge's world position, and pushes it into this frame's
    // overlay. Returns the 3 (X/Y/Z) picking handles so callers can hit-test/drag against the
    // exact rendered geometry.
    std::vector<GizmoHandle> drawSelectionGizmo(const glm::vec3& worldPos) const;
    // Returns the axis (0=X, 1=Y, 2=Z) whose handle is nearest the mouse, if within
    // the hover pixel threshold.
    std::optional<int> hitTestGizmo(const std::vector<GizmoHandle>& handles) const;

    World& world_;
    RuntimeScene& scene_;
    EditorSelection& selection_;
    AssetStore& assetStore_;
    EditorCamera& camera_;
    InputSystem& inputSystem_;
    GameWindow& window_;
    VulkanBackend& backend_;
    UndoHistory& undoHistory_;
    ObjectTransformHandle& objectTransformHandle_;
    EditorRenderer& editorRenderer_;

    bool active_ = false;
    MeshBuilderSelectMode selectMode_ = MeshBuilderSelectMode::Vertex;

    // Index into logicalVertices / logicalEdges of the currently hovered/selected element.
    std::optional<size_t> hoveredIndex_;
    std::optional<size_t> selectedIndex_;

    // Axis (0=X, 1=Y, 2=Z) of the mini gizmo currently hovered, when nothing is being dragged.
    std::optional<int> hoveredAxis_;

    bool wasLeftDown_ = false;
    std::optional<Stroke> stroke_;
};
