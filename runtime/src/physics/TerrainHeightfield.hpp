#pragma once
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include <vector>
#include "../graphics/assets/Mesh.hpp"
#include "components/BoxCollider.hpp"
#include "components/CapsuleCollider.hpp"

/*
    TerrainHeightfield

    Heightfield view over a terrain grid mesh (see terrain.md: terrains are
    ordinary grid-topology mesh assets, sculpted along Y only). Built once from
    the mesh's CPU-side positions and sampled in the mesh's local space.

    Vertex storage order is not guaranteed to be row-major (asset baking can
    reorder vertices), so the grid is reconstructed from the actual local X/Z
    coordinates.
*/
class TerrainHeightfield {
public:
    // A mesh qualifies as a terrain if it is a square grid of vertices with the
    // matching triangle count (the topology AssetPrefabs::terrainMesh produces).
    static bool isGridMesh(const Mesh& mesh) {
        size_t vertexCount = mesh.getPositions().size();
        if (vertexCount < 4) return false;

        auto side = static_cast<size_t>(std::lround(std::sqrt(static_cast<double>(vertexCount))));
        if (side * side != vertexCount) return false;

        size_t resolution = side - 1;
        if (resolution == 0) return false;

        return mesh.getIndices().size() == resolution * resolution * 6;
    }

    static std::optional<TerrainHeightfield> build(const Mesh& mesh) {
        if (!isGridMesh(mesh)) return std::nullopt;

        const auto& positions = mesh.getPositions();
        size_t vertexCount = positions.size();
        auto side = static_cast<size_t>(std::lround(std::sqrt(static_cast<double>(vertexCount))));

        constexpr float COORD_EPSILON = 1e-4f;

        std::vector<float> xs, zs;
        xs.reserve(vertexCount);
        zs.reserve(vertexCount);
        for (const auto& p: positions) {
            xs.push_back(p.x);
            zs.push_back(p.z);
        }
        std::sort(xs.begin(), xs.end());
        std::sort(zs.begin(), zs.end());
        auto dedupe = [](std::vector<float>& v) {
            v.erase(std::unique(v.begin(), v.end(), [](float a, float b) { return std::abs(a - b) < COORD_EPSILON; }),
                    v.end());
        };
        dedupe(xs);
        dedupe(zs);
        if (xs.size() != side || zs.size() != side) return std::nullopt;

        auto findIndex = [](const std::vector<float>& sorted, float value) -> size_t {
            auto it = std::lower_bound(sorted.begin(), sorted.end(), value - COORD_EPSILON);
            return static_cast<size_t>(it - sorted.begin());
        };

        TerrainHeightfield hf;
        hf.side_ = side;
        hf.xs_ = std::move(xs);
        hf.zs_ = std::move(zs);
        hf.heights_.assign(side * side, 0.0f);
        std::vector<bool> filled(side * side, false);
        for (const auto& p: positions) {
            size_t col = findIndex(hf.xs_, p.x);
            size_t row = findIndex(hf.zs_, p.z);
            if (col >= side || row >= side) return std::nullopt;
            hf.heights_[row * side + col] = p.y;
            filled[row * side + col] = true;
        }
        if (std::find(filled.begin(), filled.end(), false) != filled.end()) return std::nullopt;
        return hf;
    }

    // Bilinear height in the mesh's local space. nullopt outside the grid bounds.
    std::optional<float> heightAt(float x, float z) const {
        if (x < xs_.front() || x > xs_.back() || z < zs_.front() || z > zs_.back()) return std::nullopt;

        auto cell = [](const std::vector<float>& sorted, float value) -> size_t {
            auto it = std::upper_bound(sorted.begin(), sorted.end(), value);
            size_t i = static_cast<size_t>(it - sorted.begin());
            if (i == 0) return 0;
            return std::min(i - 1, sorted.size() - 2);
        };
        size_t col = cell(xs_, x);
        size_t row = cell(zs_, z);

        float x0 = xs_[col], x1 = xs_[col + 1];
        float z0 = zs_[row], z1 = zs_[row + 1];
        float tx = (x1 > x0) ? (x - x0) / (x1 - x0) : 0.0f;
        float tz = (z1 > z0) ? (z - z0) / (z1 - z0) : 0.0f;

        float h00 = heights_[row * side_ + col];
        float h10 = heights_[row * side_ + col + 1];
        float h01 = heights_[(row + 1) * side_ + col];
        float h11 = heights_[(row + 1) * side_ + col + 1];

        float h0 = h00 + (h10 - h00) * tx;
        float h1 = h01 + (h11 - h01) * tx;
        return h0 + (h1 - h0) * tz;
    }

private:
    TerrainHeightfield() = default;

    size_t side_ = 0;
    std::vector<float> xs_; // sorted unique local X grid coordinates
    std::vector<float> zs_; // sorted unique local Z grid coordinates
    std::vector<float> heights_; // row-major [row * side + col]
};

namespace Physics {

    // World-space points of a box collider whose penetration into the terrain
    // should be tested (the 8 OBB corners; only the deepest one matters).
    inline std::vector<glm::vec3> terrainSamplePoints(const BoxCollider& box, const glm::mat4& model) {
        std::vector<glm::vec3> points;
        points.reserve(8);
        for (int x: {-1, 1})
            for (int y: {-1, 1})
                for (int z: {-1, 1})
                    points.push_back(
                            glm::vec3(model * glm::vec4(box.center + glm::vec3(x, y, z) * box.halfExtents, 1.0f)));
        return points;
    }

    // World-space lowest point of a capsule collider (bottom of the lower hemisphere).
    inline std::vector<glm::vec3> terrainSamplePoints(const CapsuleCollider& cap, const glm::mat4& model) {
        glm::vec3 worldCenter = glm::vec3(model * glm::vec4(cap.center, 1.0f));
        glm::vec3 up = glm::normalize(glm::vec3(model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
        float scaleY = glm::length(glm::vec3(model[1]));
        float scaleXZ = glm::length(glm::vec3(model[0]));
        float halfH = cap.height * 0.5f * scaleY;
        float r = cap.radius * scaleXZ;
        glm::vec3 bottomSphere = worldCenter - up * halfH;
        return {bottomSphere + glm::vec3(0.0f, -r, 0.0f)};
    }

    // Vertical MTV pushing the sampled points out of the terrain surface, or
    // nullopt if none of them are below it. Handles the terrain entity's
    // translation and scale; rotated terrains are unsupported.
    inline std::optional<glm::vec3> computeTerrainMTV(const std::vector<glm::vec3>& worldPoints,
                                                      const TerrainHeightfield& heightfield,
                                                      const glm::mat4& terrainModel,
                                                      const glm::mat4& invTerrainModel) {
        float maxPenetration = 0.0f;
        bool hit = false;
        for (const glm::vec3& p: worldPoints) {
            glm::vec3 local = glm::vec3(invTerrainModel * glm::vec4(p, 1.0f));
            auto h = heightfield.heightAt(local.x, local.z);
            if (!h) continue;
            glm::vec3 surfaceWorld = glm::vec3(terrainModel * glm::vec4(local.x, *h, local.z, 1.0f));
            float penetration = surfaceWorld.y - p.y;
            if (penetration > maxPenetration) {
                maxPenetration = penetration;
                hit = true;
            }
        }
        if (!hit) return std::nullopt;
        return glm::vec3(0.0f, maxPenetration, 0.0f);
    }

} // namespace Physics
