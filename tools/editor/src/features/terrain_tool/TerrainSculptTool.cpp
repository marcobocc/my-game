#include "TerrainSculptTool.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <limits>
#include "../../../../../runtime/src/core/components/Transform.hpp"
#include "../../../../../runtime/src/graphics/components/Renderer.hpp"
#include "../../../../../runtime/src/graphics/debug/DebugDraw.hpp"
#include "../../rendering/VulkanBackend.hpp"
#include "core/assets/AssetLoader.hpp"
#include "physics/TerrainHeightfield.hpp"

namespace {
    // Möller-Trumbore ray-triangle intersection in a shared coordinate space.
    std::optional<float> intersectTriangle(const glm::vec3& origin,
                                           const glm::vec3& dir,
                                           const glm::vec3& v0,
                                           const glm::vec3& v1,
                                           const glm::vec3& v2) {
        constexpr float EPSILON = 1e-6f;
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 h = glm::cross(dir, edge2);
        float a = glm::dot(edge1, h);
        if (std::abs(a) < EPSILON) return std::nullopt;

        float f = 1.0f / a;
        glm::vec3 s = origin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f) return std::nullopt;

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(dir, q);
        if (v < 0.0f || u + v > 1.0f) return std::nullopt;

        float t = f * glm::dot(edge2, q);
        if (t < EPSILON) return std::nullopt;
        return t;
    }
} // namespace

bool TerrainSculptTool::isGridMesh(const Mesh& mesh) { return TerrainHeightfield::isGridMesh(mesh); }

std::optional<TerrainSculptTool::TargetMesh> TerrainSculptTool::findTarget() const {
    for (EntityHandle entity: selection_.getSelectedEntityIds()) {
        const Actor* actor = world_.getActor(entity);
        if (!actor) continue;
        const Renderer* renderer = actor->getComponent<Renderer>();
        const Transform* transform = actor->getComponent<Transform>();
        if (!renderer || !transform) continue;
        if (!assetStore_.exists(renderer->meshName)) continue;
        if (!isGridMesh(assetStore_.getAsset<Mesh>(renderer->meshName))) continue;

        glm::mat4 model = transform->getModelMatrix();
        return TargetMesh{entity, renderer->meshName, model, glm::inverse(model)};
    }
    return std::nullopt;
}

std::optional<glm::vec3> TerrainSculptTool::raycastLocal(const TargetMesh& target, glm::vec3& outNormal) const {
    const Mesh& mesh = stroke_ ? stroke_->workingMesh : assetStore_.getAsset<Mesh>(target.meshName);

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

    glm::vec3 originLocal = glm::vec3(target.invModelMatrix * glm::vec4(nearWorld, 1.0f));
    glm::vec3 farLocal = glm::vec3(target.invModelMatrix * glm::vec4(farWorld, 1.0f));
    glm::vec3 dirLocal = glm::normalize(farLocal - originLocal);

    const auto& positions = mesh.getPositions();
    const auto& indices = mesh.getIndices();
    const auto& normals = mesh.getNormals();

    std::optional<float> closestT;
    glm::vec3 hitNormal(0.0f, 1.0f, 0.0f);
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        uint32_t i0 = indices[i], i1 = indices[i + 1], i2 = indices[i + 2];
        auto t = intersectTriangle(originLocal, dirLocal, positions[i0], positions[i1], positions[i2]);
        if (t && (!closestT || *t < *closestT)) {
            closestT = t;
            if (!normals.empty()) hitNormal = normals[i0];
        }
    }
    if (!closestT) return std::nullopt;
    outNormal = hitNormal;
    return originLocal + dirLocal * (*closestT);
}

void TerrainSculptTool::recomputeNormals(Mesh& mesh) {
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

void TerrainSculptTool::applyBrush(Mesh& mesh, const glm::vec3& localHit, float deltaTime) {
    std::vector<glm::vec3> positions = mesh.getPositions();
    float speed = strength_ * deltaTime;

    if (brushOp_ == TerrainBrushOp::SMOOTH || brushOp_ == TerrainBrushOp::FLATTEN) {
        // Precompute the current heights so averaging/flattening reads a consistent snapshot.
        std::vector<glm::vec3> original = positions;
        for (size_t i = 0; i < positions.size(); ++i) {
            float dx = original[i].x - localHit.x;
            float dz = original[i].z - localHit.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist > radius_) continue;
            float falloff = 1.0f - dist / radius_;

            if (brushOp_ == TerrainBrushOp::FLATTEN) {
                positions[i].y += (localHit.y - original[i].y) * falloff * speed;
            } else {
                float sum = 0.0f;
                int count = 0;
                for (size_t j = 0; j < original.size(); ++j) {
                    float ndx = original[j].x - original[i].x;
                    float ndz = original[j].z - original[i].z;
                    if (ndx * ndx + ndz * ndz <= radius_ * radius_ * 0.04f) {
                        sum += original[j].y;
                        ++count;
                    }
                }
                if (count > 0) positions[i].y += (sum / static_cast<float>(count) - original[i].y) * falloff * speed;
            }
        }
    } else {
        float sign = brushOp_ == TerrainBrushOp::RAISE ? 1.0f : -1.0f;
        for (auto& p: positions) {
            float dx = p.x - localHit.x;
            float dz = p.z - localHit.z;
            float dist = std::sqrt(dx * dx + dz * dz);
            if (dist > radius_) continue;
            float falloff = 1.0f - dist / radius_;
            p.y += sign * falloff * speed;
        }
    }

    mesh = Mesh(mesh.name, positions, mesh.getUvs(), mesh.getColors(), mesh.getIndices(), mesh.getNormals());
    recomputeNormals(mesh);
}

void TerrainSculptTool::drawWireframe(DebugDraw& debugDraw, const TargetMesh& target, const Mesh& mesh) const {
    const auto& positions = mesh.getPositions();
    size_t vertexCount = positions.size();
    auto side = static_cast<size_t>(std::lround(std::sqrt(static_cast<double>(vertexCount))));
    if (side < 2 || side * side != vertexCount) return;

    constexpr float COORD_EPSILON = 1e-4f;

    // Vertex storage order isn't guaranteed to match the row-major generation order (asset
    // baking can reorder vertices), so reconstruct the grid from actual local X/Z coordinates
    // instead of assuming positions[row * side + col].
    std::vector<float> xs;
    std::vector<float> zs;
    xs.reserve(vertexCount);
    zs.reserve(vertexCount);
    for (const auto& p: positions) {
        xs.push_back(p.x);
        zs.push_back(p.z);
    }
    std::sort(xs.begin(), xs.end());
    std::sort(zs.begin(), zs.end());
    xs.erase(std::unique(xs.begin(), xs.end(), [](float a, float b) { return std::abs(a - b) < COORD_EPSILON; }),
             xs.end());
    zs.erase(std::unique(zs.begin(), zs.end(), [](float a, float b) { return std::abs(a - b) < COORD_EPSILON; }),
             zs.end());
    if (xs.size() != side || zs.size() != side) return;

    auto findIndex = [](const std::vector<float>& sorted, float value) -> size_t {
        auto it = std::lower_bound(sorted.begin(), sorted.end(), value - COORD_EPSILON);
        return static_cast<size_t>(it - sorted.begin());
    };

    constexpr size_t INVALID = std::numeric_limits<size_t>::max();
    std::vector<size_t> grid(side * side, INVALID);
    for (size_t i = 0; i < vertexCount; ++i) {
        size_t col = findIndex(xs, positions[i].x);
        size_t row = findIndex(zs, positions[i].z);
        if (col < side && row < side) grid[row * side + col] = i;
    }

    constexpr glm::vec3 WIRE_COLOR(0.2f, 0.9f, 0.3f);
    auto worldPoint = [&](size_t index) { return glm::vec3(target.modelMatrix * glm::vec4(positions[index], 1.0f)); };

    for (size_t row = 0; row < side; ++row) {
        for (size_t col = 0; col < side; ++col) {
            size_t current = grid[row * side + col];
            if (current == INVALID) continue;
            if (col + 1 < side) {
                size_t right = grid[row * side + col + 1];
                if (right != INVALID) debugDraw.line(worldPoint(current), worldPoint(right), WIRE_COLOR);
            }
            if (row + 1 < side) {
                size_t below = grid[(row + 1) * side + col];
                if (below != INVALID) debugDraw.line(worldPoint(current), worldPoint(below), WIRE_COLOR);
            }
        }
    }
}

void TerrainSculptTool::beginStroke(const TargetMesh& target) {
    stroke_ = Stroke{target.meshName, target.entity, assetStore_.getAsset<Mesh>(target.meshName)};
}

void TerrainSculptTool::endStroke() {
    if (!stroke_) return;
    std::string meshName = stroke_->meshName;
    Mesh finalMesh = stroke_->workingMesh;
    assetStore_.mutateAsset<Mesh>(
            meshName, [&finalMesh](Mesh& m) { m = finalMesh; }, UndoHistory::randomGroupId("Sculpt Terrain"));
    stroke_.reset();
}

void TerrainSculptTool::update(DebugDraw& debugDraw) {
    if (!active_) return;

    auto target = findTarget();
    if (!target) {
        if (stroke_) endStroke();
        wasLeftDown_ = false;
        return;
    }

    glm::vec3 localNormal;
    auto localHit = raycastLocal(*target, localNormal);
    bool leftDown = inputSystem_.isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT);

    if (localHit) {
        // Draw a ring cursor around the brush footprint (in local space, projected to world).
        constexpr int SEGMENTS = 32;
        glm::vec3 prev;
        for (int i = 0; i <= SEGMENTS; ++i) {
            float angle = static_cast<float>(i) / static_cast<float>(SEGMENTS) * glm::two_pi<float>();
            glm::vec3 local = *localHit + glm::vec3(std::cos(angle) * radius_, 0.0f, std::sin(angle) * radius_);
            glm::vec3 world = glm::vec3(target->modelMatrix * glm::vec4(local, 1.0f));
            if (i > 0) debugDraw.line(prev, world, glm::vec3(1.0f, 0.8f, 0.1f));
            prev = world;
        }
    }

    if (leftDown && localHit) {
        if (!stroke_) beginStroke(*target);
        if (stroke_ && stroke_->meshName == target->meshName) {
            applyBrush(stroke_->workingMesh, *localHit, 1.0f / 60.0f);
            backend_.getResourcesManager().updateMesh(stroke_->workingMesh);
        }
    } else if (wasLeftDown_ && !leftDown) {
        endStroke();
    }

    const Mesh& liveMesh = stroke_ ? stroke_->workingMesh : assetStore_.getAsset<Mesh>(target->meshName);
    drawWireframe(debugDraw, *target, liveMesh);

    wasLeftDown_ = leftDown;
}
