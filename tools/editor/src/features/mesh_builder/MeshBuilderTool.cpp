#include "MeshBuilderTool.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <glm/gtc/matrix_inverse.hpp>
#include <map>
#include <set>
#include "../../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../../../../../runtime/src/graphics/debug/DebugDraw.hpp"
#include "../../rendering/EditorRenderer.hpp"
#include "../../rendering/VulkanBackend.hpp"
#include "../scene_viewport/transform_handle/ObjectTransformHandle.hpp"

namespace {
    constexpr float kHoverPixelThreshold = 10.0f;
    constexpr float kPositionQuantum = 1e-4f;

    int64_t quantize(float v) { return static_cast<int64_t>(std::lround(v / kPositionQuantum)); }

    float pointSegmentDistance(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 ab = b - a;
        float lenSq = glm::dot(ab, ab);
        float t = lenSq > 1e-8f ? glm::clamp(glm::dot(p - a, ab) / lenSq, 0.0f, 1.0f) : 0.0f;
        glm::vec2 closest = a + ab * t;
        return glm::length(p - closest);
    }

    bool pointInTriangle(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b, const glm::vec2& c) {
        float d1 = (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
        float d2 = (p.x - c.x) * (b.y - c.y) - (b.x - c.x) * (p.y - c.y);
        float d3 = (p.x - a.x) * (c.y - a.y) - (c.x - a.x) * (p.y - a.y);
        bool hasNeg = (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
        bool hasPos = (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
        return !(hasNeg && hasPos);
    }

    std::vector<uint32_t> buildRawToLogical(size_t vertexCount,
                                            const std::vector<std::vector<uint32_t>>& logicalVertices) {
        std::vector<uint32_t> rawToLogical(vertexCount, 0);
        for (uint32_t group = 0; group < logicalVertices.size(); ++group)
            for (uint32_t raw: logicalVertices[group])
                rawToLogical[raw] = group;
        return rawToLogical;
    }

    // Expands a seed set of raw vertex indices to include every raw vertex sharing a position
    // (logical vertex group) with any of them, so dragging never tears the mesh at seams.
    std::vector<uint32_t> expandToLogicalGroups(const std::vector<std::vector<uint32_t>>& logicalVertices,
                                                const std::vector<uint32_t>& rawToLogical,
                                                const std::vector<uint32_t>& seedRaw) {
        std::set<uint32_t> groups;
        for (uint32_t r: seedRaw)
            groups.insert(rawToLogical[r]);
        std::vector<uint32_t> result;
        for (uint32_t g: groups)
            result.insert(result.end(), logicalVertices[g].begin(), logicalVertices[g].end());
        return result;
    }
} // namespace

std::optional<MeshBuilderTool::TargetMesh> MeshBuilderTool::findTarget() const {
    for (EntityHandle entity: selection_.getSelectedEntityIds()) {
        const Actor* actor = world_.getActor(entity);
        if (!actor) continue;
        const Renderer* renderer = actor->getComponent<Renderer>();
        const Transform* transform = actor->getComponent<Transform>();
        if (!renderer || !transform) continue;
        if (!assetStore_.exists(renderer->meshName)) continue;
        if (!assetStore_.isMutable(renderer->meshName)) continue;

        glm::mat4 model = transform->getModelMatrix();
        return TargetMesh{entity, renderer->meshName, model, glm::inverse(model)};
    }
    return std::nullopt;
}

std::vector<std::vector<uint32_t>> MeshBuilderTool::buildLogicalVertices(const Mesh& mesh) {
    std::map<std::tuple<int64_t, int64_t, int64_t>, std::vector<uint32_t>> groups;
    const auto& positions = mesh.getPositions();
    for (uint32_t i = 0; i < positions.size(); ++i) {
        auto key = std::make_tuple(quantize(positions[i].x), quantize(positions[i].y), quantize(positions[i].z));
        groups[key].push_back(i);
    }
    std::vector<std::vector<uint32_t>> logicalVertices;
    logicalVertices.reserve(groups.size());
    for (auto& [key, indices]: groups)
        logicalVertices.push_back(std::move(indices));
    return logicalVertices;
}

std::vector<MeshBuilderTool::LogicalEdge>
MeshBuilderTool::buildLogicalEdges(const Mesh& mesh, const std::vector<std::vector<uint32_t>>& logicalVertices) {
    std::vector<uint32_t> rawToLogical = buildRawToLogical(mesh.getPositions().size(), logicalVertices);

    std::set<std::pair<uint32_t, uint32_t>> seen;
    std::vector<LogicalEdge> edges;
    const auto& indices = mesh.getIndices();
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t tri[3] = {indices[i], indices[i + 1], indices[i + 2]};
        for (int e = 0; e < 3; ++e) {
            uint32_t a = rawToLogical[tri[e]];
            uint32_t b = rawToLogical[tri[(e + 1) % 3]];
            if (a == b) continue;
            auto key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
            if (seen.insert(key).second) edges.push_back(LogicalEdge{key.first, key.second});
        }
    }
    return edges;
}

std::vector<MeshBuilderTool::LogicalFace>
MeshBuilderTool::buildLogicalFaces(const Mesh& mesh, const std::vector<std::vector<uint32_t>>& logicalVertices) {
    std::vector<uint32_t> rawToLogical = buildRawToLogical(mesh.getPositions().size(), logicalVertices);

    const auto& positions = mesh.getPositions();
    const auto& indices = mesh.getIndices();
    size_t triCount = indices.size() / 3;

    std::vector<glm::vec3> triNormals(triCount);
    for (size_t t = 0; t < triCount; ++t) {
        uint32_t i0 = indices[t * 3], i1 = indices[t * 3 + 1], i2 = indices[t * 3 + 2];
        glm::vec3 n = glm::cross(positions[i2] - positions[i0], positions[i1] - positions[i0]);
        triNormals[t] = glm::length(n) > 1e-8f ? glm::normalize(n) : glm::vec3(0.0f);
    }

    // Logical-vertex-pair edge -> triangles sharing it, used to flood-fill coplanar groups.
    std::map<std::pair<uint32_t, uint32_t>, std::vector<uint32_t>> edgeToTris;
    for (size_t t = 0; t < triCount; ++t) {
        uint32_t tri[3] = {
                rawToLogical[indices[t * 3]], rawToLogical[indices[t * 3 + 1]], rawToLogical[indices[t * 3 + 2]]};
        for (int e = 0; e < 3; ++e) {
            uint32_t a = tri[e];
            uint32_t b = tri[(e + 1) % 3];
            auto key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
            edgeToTris[key].push_back(static_cast<uint32_t>(t));
        }
    }

    constexpr float kCoplanarDot = 0.999f;
    std::vector<bool> visited(triCount, false);
    std::vector<LogicalFace> faces;
    for (size_t start = 0; start < triCount; ++start) {
        if (visited[start]) continue;
        visited[start] = true;
        std::vector<uint32_t> group{static_cast<uint32_t>(start)};
        std::vector<uint32_t> stack{static_cast<uint32_t>(start)};
        while (!stack.empty()) {
            uint32_t t = stack.back();
            stack.pop_back();
            uint32_t tri[3] = {
                    rawToLogical[indices[t * 3]], rawToLogical[indices[t * 3 + 1]], rawToLogical[indices[t * 3 + 2]]};
            for (int e = 0; e < 3; ++e) {
                uint32_t a = tri[e];
                uint32_t b = tri[(e + 1) % 3];
                auto key = a < b ? std::make_pair(a, b) : std::make_pair(b, a);
                for (uint32_t other: edgeToTris[key]) {
                    if (visited[other]) continue;
                    if (glm::dot(triNormals[t], triNormals[other]) > kCoplanarDot) {
                        visited[other] = true;
                        group.push_back(other);
                        stack.push_back(other);
                    }
                }
            }
        }

        LogicalFace face;
        std::set<uint32_t> vertSet;
        for (uint32_t t: group) {
            face.triangleStarts.push_back(static_cast<uint32_t>(t * 3));
            vertSet.insert(indices[t * 3]);
            vertSet.insert(indices[t * 3 + 1]);
            vertSet.insert(indices[t * 3 + 2]);
        }
        face.vertices.assign(vertSet.begin(), vertSet.end());
        faces.push_back(std::move(face));
    }
    return faces;
}

void MeshBuilderTool::ensureUniqueMesh(TargetMesh& target, const std::string& mutationGroup) {
    const std::string ownedMarker = "__mb" + std::to_string(target.entity) + ".";
    if (target.meshName.find(ownedMarker) != std::string::npos) return; // already an entity-owned fork

    std::filesystem::path original(target.meshName);
    std::string newName =
            (original.parent_path() /
             (original.stem().string() + ownedMarker.substr(0, ownedMarker.size() - 1) + original.extension().string()))
                    .string();

    if (!assetStore_.exists(newName)) {
        Mesh forked = assetStore_.getAsset<Mesh>(target.meshName);
        forked.name = newName;
        assetStore_.createAssetFile<Mesh>(forked, mutationGroup);
    }

    scene_.getObject(target.entity)
            .mutateComponent<Renderer>([&newName](Renderer& r) { r.meshName = newName; }, mutationGroup);

    target.meshName = newName;
}

std::vector<GizmoHandle> MeshBuilderTool::drawSelectionGizmo(const glm::vec3& worldPos) const {
    auto result =
            objectTransformHandle_.buildTranslationGizmo(worldPos, camera_.getCamera(), camera_.getCameraTransform());
    editorRenderer_.appendOverlayGizmoVertices(result.visualization);
    return result.pickingHandles;
}

std::optional<int> MeshBuilderTool::hitTestGizmo(const std::vector<GizmoHandle>& handles) const {
    auto [mx, my] = inputSystem_.getMousePosition();
    glm::vec2 mouse(static_cast<float>(mx), static_cast<float>(my));

    std::optional<int> best;
    float bestDist = kHoverPixelThreshold;
    for (int i = 0; i < static_cast<int>(handles.size()); ++i) {
        auto base = worldToScreen(handles[i].base);
        auto tip = worldToScreen(handles[i].tip);
        if (!base || !tip) continue;
        float dist = pointSegmentDistance(mouse, *base, *tip);
        if (dist < bestDist) {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

std::optional<glm::vec2> MeshBuilderTool::worldToScreen(const glm::vec3& world) const {
    auto sv = window_.getSceneViewport();
    glm::mat4 vp = camera_.getCamera().getProjectionMatrix() * camera_.getCameraTransform().getViewMatrix();
    glm::vec4 clip = vp * glm::vec4(world, 1.0f);
    if (clip.w <= 1e-6f) return std::nullopt;
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.x < -1.0f || ndc.x > 1.0f || ndc.y < -1.0f || ndc.y > 1.0f) return std::nullopt;
    float screenX = static_cast<float>(sv.x) + (ndc.x * 0.5f + 0.5f) * static_cast<float>(sv.width);
    float screenY = static_cast<float>(sv.y) + (ndc.y * 0.5f + 0.5f) * static_cast<float>(sv.height);
    return glm::vec2(screenX, screenY);
}

std::optional<std::pair<glm::vec3, glm::vec3>> MeshBuilderTool::mouseRayWorld() const {
    auto [mx, my] = inputSystem_.getMousePosition();
    auto sv = window_.getSceneViewport();
    float ndcX = (static_cast<float>(mx) - static_cast<float>(sv.x)) / static_cast<float>(sv.width);
    float ndcY = (static_cast<float>(my) - static_cast<float>(sv.y)) / static_cast<float>(sv.height);
    if (ndcX < 0.0f || ndcX > 1.0f || ndcY < 0.0f || ndcY > 1.0f) return std::nullopt;

    float clipX = ndcX * 2.0f - 1.0f;
    float clipY = ndcY * 2.0f - 1.0f;
    glm::mat4 invVP =
            glm::inverse(camera_.getCamera().getProjectionMatrix() * camera_.getCameraTransform().getViewMatrix());
    glm::vec4 nearPoint = invVP * glm::vec4(clipX, clipY, -1.0f, 1.0f);
    glm::vec4 farPoint = invVP * glm::vec4(clipX, clipY, 1.0f, 1.0f);
    glm::vec3 nearWorld = glm::vec3(nearPoint) / nearPoint.w;
    glm::vec3 farWorld = glm::vec3(farPoint) / farPoint.w;
    return std::make_pair(nearWorld, glm::normalize(farWorld - nearWorld));
}

void MeshBuilderTool::recomputeNormals(Mesh& mesh) {
    std::vector<glm::vec3> normals(mesh.getPositions().size(), glm::vec3(0.0f));
    const auto& positions = mesh.getPositions();
    const auto& indices = mesh.getIndices();
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        glm::vec3 n = glm::cross(positions[i2] - positions[i0], positions[i1] - positions[i0]);
        normals[i0] += n;
        normals[i1] += n;
        normals[i2] += n;
    }
    for (auto& n: normals) {
        n = glm::length(n) > 1e-8f ? glm::normalize(n) : glm::vec3(0.0f, 1.0f, 0.0f);
    }
    mesh = Mesh(mesh.name, positions, mesh.getUvs(), mesh.getColors(), indices, normals);
}

void MeshBuilderTool::beginStroke(const TargetMesh& target,
                                  const std::vector<uint32_t>& draggedIndices,
                                  const glm::vec3& pivotWorld,
                                  const glm::vec3& axisDir,
                                  const std::string& mutationGroup) {
    Mesh working = assetStore_.getAsset<Mesh>(target.meshName);
    std::vector<glm::vec3> initial;
    initial.reserve(draggedIndices.size());
    for (uint32_t idx: draggedIndices)
        initial.push_back(working.getPositions()[idx]);

    // Drag plane: contains the pivot, faces the camera as much as possible while
    // remaining perpendicular to the constrained axis (mirrors ObjectTransformHandle).
    glm::vec3 camDir = glm::normalize(camera_.getCameraTransform().position - pivotWorld);
    glm::vec3 planeNormal = camDir - glm::dot(camDir, axisDir) * axisDir;
    planeNormal = glm::length(planeNormal) > 1e-5f ? glm::normalize(planeNormal) : camera_.getCameraTransform().getUp();

    glm::vec3 planePoint = pivotWorld;
    if (auto ray = mouseRayWorld()) {
        const auto& [origin, dir] = *ray;
        float denom = glm::dot(dir, planeNormal);
        if (std::abs(denom) > 1e-6f) {
            float t = glm::dot(pivotWorld - origin, planeNormal) / denom;
            planePoint = origin + dir * t;
        }
    }

    stroke_ = Stroke{target.meshName,
                     target.entity,
                     std::move(working),
                     draggedIndices,
                     std::move(initial),
                     pivotWorld,
                     axisDir,
                     planePoint,
                     planeNormal,
                     mutationGroup};
}

void MeshBuilderTool::endStroke() {
    if (!stroke_) return;
    std::string meshName = stroke_->meshName;
    Mesh finalMesh = stroke_->workingMesh;
    std::string mutationGroup = stroke_->mutationGroup;
    assetStore_.mutateAsset<Mesh>(meshName, [&finalMesh](Mesh& m) { m = finalMesh; }, mutationGroup);
    stroke_.reset();
}

void MeshBuilderTool::drawHighlights(DebugDraw& debugDraw,
                                     const TargetMesh& target,
                                     const Mesh& mesh,
                                     const std::vector<std::vector<uint32_t>>& logicalVertices,
                                     const std::vector<LogicalEdge>& logicalEdges,
                                     const std::vector<LogicalFace>& logicalFaces) const {
    const auto& positions = mesh.getPositions();
    const auto& indices = mesh.getIndices();
    auto worldPoint = [&](uint32_t raw) { return glm::vec3(target.modelMatrix * glm::vec4(positions[raw], 1.0f)); };

    // Dim wireframe of the whole mesh for context.
    constexpr glm::vec3 DIM_COLOR(0.35f, 0.35f, 0.4f);
    for (const auto& edge: logicalEdges) {
        glm::vec3 a = worldPoint(logicalVertices[edge.a].front());
        glm::vec3 b = worldPoint(logicalVertices[edge.b].front());
        debugDraw.line(a, b, DIM_COLOR);
    }

    constexpr glm::vec3 HOVER_COLOR(1.0f, 1.0f, 1.0f);
    constexpr glm::vec3 SELECTED_COLOR(1.0f, 0.55f, 0.05f);
    constexpr float CROSS_SIZE = 0.15f;

    auto drawVertexMarker = [&](uint32_t raw, const glm::vec3& color) {
        glm::vec3 p = worldPoint(raw);
        debugDraw.line(p - glm::vec3(CROSS_SIZE, 0, 0), p + glm::vec3(CROSS_SIZE, 0, 0), color);
        debugDraw.line(p - glm::vec3(0, CROSS_SIZE, 0), p + glm::vec3(0, CROSS_SIZE, 0), color);
        debugDraw.line(p - glm::vec3(0, 0, CROSS_SIZE), p + glm::vec3(0, 0, CROSS_SIZE), color);
    };

    auto drawFaceOutline = [&](const LogicalFace& face, const glm::vec3& color) {
        for (uint32_t triStart: face.triangleStarts) {
            glm::vec3 a = worldPoint(indices[triStart]);
            glm::vec3 b = worldPoint(indices[triStart + 1]);
            glm::vec3 c = worldPoint(indices[triStart + 2]);
            debugDraw.line(a, b, color);
            debugDraw.line(b, c, color);
            debugDraw.line(c, a, color);
        }
    };

    if (selectMode_ == MeshBuilderSelectMode::Vertex) {
        if (hoveredIndex_ && *hoveredIndex_ < logicalVertices.size())
            drawVertexMarker(logicalVertices[*hoveredIndex_].front(), HOVER_COLOR);
        if (selectedIndex_ && *selectedIndex_ < logicalVertices.size())
            drawVertexMarker(logicalVertices[*selectedIndex_].front(), SELECTED_COLOR);
    } else if (selectMode_ == MeshBuilderSelectMode::Edge) {
        if (hoveredIndex_ && *hoveredIndex_ < logicalEdges.size()) {
            const auto& e = logicalEdges[*hoveredIndex_];
            debugDraw.line(
                    worldPoint(logicalVertices[e.a].front()), worldPoint(logicalVertices[e.b].front()), HOVER_COLOR);
        }
        if (selectedIndex_ && *selectedIndex_ < logicalEdges.size()) {
            const auto& e = logicalEdges[*selectedIndex_];
            debugDraw.line(
                    worldPoint(logicalVertices[e.a].front()), worldPoint(logicalVertices[e.b].front()), SELECTED_COLOR);
        }
    } else {
        if (hoveredIndex_ && *hoveredIndex_ < logicalFaces.size())
            drawFaceOutline(logicalFaces[*hoveredIndex_], HOVER_COLOR);
        if (selectedIndex_ && *selectedIndex_ < logicalFaces.size())
            drawFaceOutline(logicalFaces[*selectedIndex_], SELECTED_COLOR);
    }
}

void MeshBuilderTool::update(DebugDraw& debugDraw) {
    if (!active_) return;

    auto target = findTarget();
    if (!target) {
        if (stroke_) endStroke();
        hoveredIndex_.reset();
        selectedIndex_.reset();
        wasLeftDown_ = false;
        return;
    }

    const Mesh& mesh = stroke_ ? stroke_->workingMesh : assetStore_.getAsset<Mesh>(target->meshName);
    auto logicalVertices = buildLogicalVertices(mesh);
    auto logicalEdges = buildLogicalEdges(mesh, logicalVertices);
    auto logicalFaces = buildLogicalFaces(mesh, logicalVertices);
    auto rawToLogical = buildRawToLogical(mesh.getPositions().size(), logicalVertices);

    bool leftDown = inputSystem_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);
    auto [mx, my] = inputSystem_.getMousePosition();
    glm::vec2 mouse(static_cast<float>(mx), static_cast<float>(my));

    // Local-space pivot of element `index` in the current select mode, given the mesh in use.
    auto elementLocalPivot = [&](const Mesh& m, size_t index) -> std::optional<glm::vec3> {
        if (selectMode_ == MeshBuilderSelectMode::Vertex) {
            if (index >= logicalVertices.size()) return std::nullopt;
            return m.getPositions()[logicalVertices[index].front()];
        }
        if (selectMode_ == MeshBuilderSelectMode::Edge) {
            if (index >= logicalEdges.size()) return std::nullopt;
            const auto& e = logicalEdges[index];
            return (m.getPositions()[logicalVertices[e.a].front()] + m.getPositions()[logicalVertices[e.b].front()]) *
                   0.5f;
        }
        if (index >= logicalFaces.size()) return std::nullopt;
        const auto& face = logicalFaces[index];
        glm::vec3 sum{0.0f};
        for (uint32_t raw: face.vertices)
            sum += m.getPositions()[raw];
        return sum / static_cast<float>(face.vertices.size());
    };

    // Raw vertex indices (expanded to full logical groups, so dragging never tears the mesh)
    // that make up element `index` in the current select mode.
    auto elementDraggedRaw = [&](size_t index) -> std::vector<uint32_t> {
        std::vector<uint32_t> seed;
        if (selectMode_ == MeshBuilderSelectMode::Vertex) {
            seed = logicalVertices[index];
        } else if (selectMode_ == MeshBuilderSelectMode::Edge) {
            const auto& e = logicalEdges[index];
            seed = logicalVertices[e.a];
            seed.insert(seed.end(), logicalVertices[e.b].begin(), logicalVertices[e.b].end());
        } else {
            seed = logicalFaces[index].vertices;
        }
        return expandToLogicalGroups(logicalVertices, rawToLogical, seed);
    };

    auto elementCount = [&]() -> size_t {
        if (selectMode_ == MeshBuilderSelectMode::Vertex) return logicalVertices.size();
        if (selectMode_ == MeshBuilderSelectMode::Edge) return logicalEdges.size();
        return logicalFaces.size();
    };

    // World-space position of the current selection, used as the mini gizmo's pivot.
    auto selectionWorldPos = [&]() -> std::optional<glm::vec3> {
        if (!selectedIndex_) return std::nullopt;
        auto local = elementLocalPivot(mesh, *selectedIndex_);
        if (!local) return std::nullopt;
        return glm::vec3(target->modelMatrix * glm::vec4(*local, 1.0f));
    };

    if (!stroke_) {
        hoveredIndex_.reset();
        hoveredAxis_.reset();

        std::optional<glm::vec3> gizmoWorldPos = selectionWorldPos();
        std::vector<GizmoHandle> gizmoHandles;
        if (gizmoWorldPos) {
            gizmoHandles = drawSelectionGizmo(*gizmoWorldPos);
            hoveredAxis_ = hitTestGizmo(gizmoHandles);
        }

        // Only hover-test elements when not hovering the gizmo, so gizmo handles take priority.
        if (!hoveredAxis_) {
            float bestDist = kHoverPixelThreshold;
            if (selectMode_ == MeshBuilderSelectMode::Vertex) {
                for (size_t i = 0; i < logicalVertices.size(); ++i) {
                    glm::vec3 world = glm::vec3(target->modelMatrix *
                                                glm::vec4(mesh.getPositions()[logicalVertices[i].front()], 1.0f));
                    auto screen = worldToScreen(world);
                    if (!screen) continue;
                    float dist = glm::length(*screen - mouse);
                    if (dist < bestDist) {
                        bestDist = dist;
                        hoveredIndex_ = i;
                    }
                }
            } else if (selectMode_ == MeshBuilderSelectMode::Edge) {
                for (size_t i = 0; i < logicalEdges.size(); ++i) {
                    const auto& e = logicalEdges[i];
                    glm::vec3 wa = glm::vec3(target->modelMatrix *
                                             glm::vec4(mesh.getPositions()[logicalVertices[e.a].front()], 1.0f));
                    glm::vec3 wb = glm::vec3(target->modelMatrix *
                                             glm::vec4(mesh.getPositions()[logicalVertices[e.b].front()], 1.0f));
                    auto sa = worldToScreen(wa);
                    auto sb = worldToScreen(wb);
                    if (!sa || !sb) continue;
                    float dist = pointSegmentDistance(mouse, *sa, *sb);
                    if (dist < bestDist) {
                        bestDist = dist;
                        hoveredIndex_ = i;
                    }
                }
            } else {
                const auto& indices = mesh.getIndices();
                float bestDepth = std::numeric_limits<float>::max();
                for (size_t i = 0; i < logicalFaces.size(); ++i) {
                    for (uint32_t triStart: logicalFaces[i].triangleStarts) {
                        glm::vec3 wa = glm::vec3(target->modelMatrix *
                                                 glm::vec4(mesh.getPositions()[indices[triStart]], 1.0f));
                        glm::vec3 wb = glm::vec3(target->modelMatrix *
                                                 glm::vec4(mesh.getPositions()[indices[triStart + 1]], 1.0f));
                        glm::vec3 wc = glm::vec3(target->modelMatrix *
                                                 glm::vec4(mesh.getPositions()[indices[triStart + 2]], 1.0f));
                        auto sa = worldToScreen(wa);
                        auto sb = worldToScreen(wb);
                        auto sc = worldToScreen(wc);
                        if (!sa || !sb || !sc) continue;
                        if (!pointInTriangle(mouse, *sa, *sb, *sc)) continue;
                        float depth = glm::length(camera_.getCameraTransform().position - (wa + wb + wc) / 3.0f);
                        if (depth < bestDepth) {
                            bestDepth = depth;
                            hoveredIndex_ = i;
                        }
                    }
                }
            }
        }

        if (leftDown && !wasLeftDown_) {
            if (hoveredAxis_ && gizmoWorldPos) {
                std::vector<uint32_t> dragged = elementDraggedRaw(*selectedIndex_);
                const GizmoHandle& handle = gizmoHandles[*hoveredAxis_];
                glm::vec3 axisDir = glm::normalize(handle.tip - handle.base);
                std::string mutationGroup = UndoHistory::randomGroupId("Move Mesh Vertex");
                ensureUniqueMesh(*target, mutationGroup);
                beginStroke(*target, dragged, *gizmoWorldPos, axisDir, mutationGroup);
            } else if (hoveredIndex_) {
                // Selecting an element only moves the gizmo there; dragging happens via its axis handles.
                selectedIndex_ = hoveredIndex_;
            }
        }
    } else if (leftDown) {
        auto ray = mouseRayWorld();
        if (ray) {
            const auto& [origin, dir] = *ray;
            float denom = glm::dot(dir, stroke_->planeNormal);
            if (std::abs(denom) > 1e-6f) {
                float t = glm::dot(stroke_->planePoint - origin, stroke_->planeNormal) / denom;
                glm::vec3 hitWorld = origin + dir * t;

                float startT = glm::dot(stroke_->planePoint - stroke_->pivotWorld, stroke_->axisDir);
                float currentT = glm::dot(hitWorld - stroke_->pivotWorld, stroke_->axisDir);
                glm::vec3 worldDelta = stroke_->axisDir * (currentT - startT);
                glm::vec3 localDelta = glm::mat3(target->invModelMatrix) * worldDelta;

                std::vector<glm::vec3> positions = stroke_->workingMesh.getPositions();
                for (size_t k = 0; k < stroke_->draggedIndices.size(); ++k)
                    positions[stroke_->draggedIndices[k]] = stroke_->initialLocalPositions[k] + localDelta;

                stroke_->workingMesh = Mesh(stroke_->workingMesh.name,
                                            positions,
                                            stroke_->workingMesh.getUvs(),
                                            stroke_->workingMesh.getColors(),
                                            stroke_->workingMesh.getIndices(),
                                            stroke_->workingMesh.getNormals());
                recomputeNormals(stroke_->workingMesh);
                backend_.getResourcesManager().updateMesh(stroke_->workingMesh);
            }
        }
    }

    if (wasLeftDown_ && !leftDown && stroke_) endStroke();

    const Mesh& liveMesh = stroke_ ? stroke_->workingMesh : assetStore_.getAsset<Mesh>(target->meshName);
    auto liveLogicalVertices = stroke_ ? buildLogicalVertices(liveMesh) : logicalVertices;
    auto liveLogicalEdges = stroke_ ? buildLogicalEdges(liveMesh, liveLogicalVertices) : logicalEdges;
    auto liveLogicalFaces = stroke_ ? buildLogicalFaces(liveMesh, liveLogicalVertices) : logicalFaces;
    drawHighlights(debugDraw, *target, liveMesh, liveLogicalVertices, liveLogicalEdges, liveLogicalFaces);

    // While dragging, the gizmo must be redrawn each frame at the moving pivot; when merely
    // selected (not dragging), it was already drawn above at hover/select-test time.
    if (stroke_ && selectedIndex_ && *selectedIndex_ < elementCount()) {
        auto local = elementLocalPivot(liveMesh, *selectedIndex_);
        if (local) drawSelectionGizmo(glm::vec3(target->modelMatrix * glm::vec4(*local, 1.0f)));
    }

    wasLeftDown_ = leftDown;
}
