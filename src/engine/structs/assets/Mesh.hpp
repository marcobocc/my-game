#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "Asset.hpp"
#include "utils/math/AABB.hpp"
#include "utils/math/BoundingSphere.hpp"

class Mesh final : public Asset {
public:
    Mesh(const std::string& name,
         std::vector<glm::vec3> positions,
         std::vector<glm::vec2> uvs,
         std::vector<glm::vec3> colors,
         std::vector<uint32_t> indices,
         std::vector<glm::vec3> normals) :
        Asset(name),
        positions_(std::move(positions)),
        uvs_(std::move(uvs)),
        colors_(std::move(colors)),
        indices_(std::move(indices)),
        normals_(std::move(normals)),
        tangents_(computeTangents(positions_, uvs_, indices_)),
        aabb_(positions_),
        boundingSphere_(positions_) {}

    bool hasIndices() const { return !indices_.empty(); }
    bool hasUvs() const { return !uvs_.empty(); }
    bool hasColors() const { return !colors_.empty(); }
    bool hasNormals() const { return !normals_.empty(); }
    bool hasTangents() const { return !tangents_.empty(); }
    size_t getVertexCount() const { return positions_.size(); }

    const std::vector<glm::vec3>& getPositions() const { return positions_; }
    const std::vector<glm::vec2>& getUvs() const { return uvs_; }
    const std::vector<glm::vec3>& getColors() const { return colors_; }
    const std::vector<uint32_t>& getIndices() const { return indices_; }
    const std::vector<glm::vec3>& getNormals() const { return normals_; }
    const std::vector<glm::vec4>& getTangents() const { return tangents_; }
    AABB getAABB() const { return aabb_; }
    BoundingSphere getBoundingSphere() const { return boundingSphere_; }

private:
    static std::vector<glm::vec4> computeTangents(const std::vector<glm::vec3>& positions,
                                                  const std::vector<glm::vec2>& uvs,
                                                  const std::vector<uint32_t>& indices) {
        std::vector<glm::vec4> tangents(positions.size(), glm::vec4(0.0f));

        if (indices.empty() || uvs.empty()) {
            for (auto& t: tangents) {
                t = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
            return tangents;
        }

        for (size_t i = 0; i < indices.size(); i += 3) {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            glm::vec3 p0 = positions[i0], p1 = positions[i1], p2 = positions[i2];
            glm::vec2 uv0 = uvs[i0], uv1 = uvs[i1], uv2 = uvs[i2];

            glm::vec3 edge1 = p1 - p0, edge2 = p2 - p0;
            glm::vec2 deltaUv1 = uv1 - uv0, deltaUv2 = uv2 - uv0;

            float det = deltaUv1.x * deltaUv2.y - deltaUv2.x * deltaUv1.y;
            if (glm::abs(det) < 1e-6f) det = 1.0f;

            float invDet = 1.0f / det;
            glm::vec3 tangent = (edge1 * deltaUv2.y - edge2 * deltaUv1.y) * invDet;

            tangents[i0] += glm::vec4(tangent, 0.0f);
            tangents[i1] += glm::vec4(tangent, 0.0f);
            tangents[i2] += glm::vec4(tangent, 0.0f);
        }

        for (auto& t: tangents) {
            if (glm::length(glm::vec3(t)) > 1e-6f) {
                t = glm::vec4(glm::normalize(glm::vec3(t)), 1.0f);
            } else {
                t = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            }
        }

        return tangents;
    }

    std::vector<glm::vec3> positions_;
    std::vector<glm::vec2> uvs_;
    std::vector<glm::vec3> colors_;
    std::vector<uint32_t> indices_;
    std::vector<glm::vec3> normals_;
    std::vector<glm::vec4> tangents_;
    AABB aabb_;
    BoundingSphere boundingSphere_;
};
