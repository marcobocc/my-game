#pragma once
#include <array>
#include <glm/glm.hpp>
#include <limits>
#include <optional>
#include "ConvexHull.hpp"
#include "components/BoxCollider.hpp"
#include "components/CapsuleCollider.hpp"

// SAT-based MTV tests between the box/capsule colliders and a mesh collider's
// convex hull. Uses the hull's face normals (already computed by ConvexHull)
// as separating axes, plus the box's own face normals for the box-vs-hull
// case. This mirrors the approximate nature of computeCapsuleBoxMTV (no
// edge-edge axes) rather than the full 15-axis test used for box-vs-box.
namespace Physics {
    namespace MeshCollisionDetail {
        inline std::vector<glm::vec3> worldHullVertices(const ConvexHull& hull, const glm::mat4& matHull) {
            std::vector<glm::vec3> verts;
            verts.reserve(hull.vertices.size());
            for (const glm::vec3& v: hull.vertices)
                verts.push_back(glm::vec3(matHull * glm::vec4(v, 1.0f)));
            return verts;
        }

        inline std::vector<glm::vec3> worldHullNormals(const ConvexHull& hull, const glm::mat4& matHull) {
            glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(matHull)));
            std::vector<glm::vec3> normals;
            normals.reserve(hull.faceNormals.size());
            for (const glm::vec3& n: hull.faceNormals) {
                glm::vec3 world = normalMat * n;
                float len = glm::length(world);
                if (len > 1e-6f) normals.push_back(world / len);
            }
            return normals;
        }

        inline glm::vec3 centroidOf(const std::vector<glm::vec3>& points) {
            glm::vec3 sum(0.0f);
            for (const glm::vec3& p: points)
                sum += p;
            return points.empty() ? sum : sum / static_cast<float>(points.size());
        }

        inline std::pair<float, float> projectPoints(const std::vector<glm::vec3>& points, const glm::vec3& axis) {
            float lo = std::numeric_limits<float>::max();
            float hi = std::numeric_limits<float>::lowest();
            for (const glm::vec3& p: points) {
                float d = glm::dot(p, axis);
                lo = std::min(lo, d);
                hi = std::max(hi, d);
            }
            return {lo, hi};
        }
    } // namespace MeshCollisionDetail

    // Box vs convex hull MTV. Returns the push to move the box out of the hull, or
    // nullopt if they don't overlap.
    inline std::optional<glm::vec3> computeBoxHullMTV(const BoxCollider& box,
                                                      const glm::mat4& matBox,
                                                      const ConvexHull& hull,
                                                      const glm::mat4& matHull) {
        if (hull.vertices.empty() || hull.faces.empty()) return std::nullopt;

        glm::vec3 boxCenter = glm::vec3(matBox * glm::vec4(box.center, 1.0f));
        std::array<glm::vec3, 3> boxAx{};
        std::array<float, 3> boxHe{};
        for (int i = 0; i < 3; ++i) {
            glm::vec3 col = glm::vec3(matBox[i]);
            float len = glm::length(col);
            boxAx.at(i) = (len > 1e-6f) ? col / len : glm::vec3(0.0f);
            boxHe.at(i) = box.halfExtents[i] * len;
        }

        std::vector<glm::vec3> hullVerts = MeshCollisionDetail::worldHullVertices(hull, matHull);
        std::vector<glm::vec3> hullNormals = MeshCollisionDetail::worldHullNormals(hull, matHull);
        glm::vec3 hullCentroid = MeshCollisionDetail::centroidOf(hullVerts);

        std::vector<glm::vec3> axes(boxAx.begin(), boxAx.end());
        axes.insert(axes.end(), hullNormals.begin(), hullNormals.end());

        float bestOverlap = std::numeric_limits<float>::max();
        glm::vec3 bestAxis{0.0f};

        for (const glm::vec3& axis: axes) {
            float len = glm::length(axis);
            if (len < 1e-6f) continue;
            glm::vec3 a = axis / len;

            float boxRadius = 0.0f;
            for (int i = 0; i < 3; ++i)
                boxRadius += std::abs(glm::dot(boxAx[i], a)) * boxHe[i];
            float boxCenterProj = glm::dot(boxCenter, a);
            float boxMin = boxCenterProj - boxRadius;
            float boxMax = boxCenterProj + boxRadius;

            auto [hullMin, hullMax] = MeshCollisionDetail::projectPoints(hullVerts, a);

            float overlap = std::min(boxMax, hullMax) - std::max(boxMin, hullMin);
            if (overlap <= 0.0f) return std::nullopt;
            if (overlap < bestOverlap) {
                bestOverlap = overlap;
                float sign = (boxCenterProj - glm::dot(hullCentroid, a) >= 0.0f) ? 1.0f : -1.0f;
                bestAxis = a * sign;
            }
        }

        if (bestOverlap == std::numeric_limits<float>::max()) return std::nullopt;
        return bestAxis * bestOverlap;
    }

    // Capsule vs convex hull MTV. Returns the push to move the capsule out of the
    // hull, or nullopt if they don't overlap.
    inline std::optional<glm::vec3> computeCapsuleHullMTV(const CapsuleCollider& cap,
                                                          const glm::mat4& matCap,
                                                          const ConvexHull& hull,
                                                          const glm::mat4& matHull) {
        if (hull.vertices.empty() || hull.faces.empty()) return std::nullopt;

        glm::vec3 capWorldCenter = glm::vec3(matCap * glm::vec4(cap.center, 1.0f));
        glm::vec3 up = glm::normalize(glm::vec3(matCap * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
        float scaleY = glm::length(glm::vec3(matCap[1]));
        float scaleXZ = glm::length(glm::vec3(matCap[0]));
        float halfH = cap.height * 0.5f * scaleY;
        float r = cap.radius * scaleXZ;
        glm::vec3 segA = capWorldCenter + up * halfH;
        glm::vec3 segB = capWorldCenter - up * halfH;

        std::vector<glm::vec3> hullVerts = MeshCollisionDetail::worldHullVertices(hull, matHull);
        std::vector<glm::vec3> hullNormals = MeshCollisionDetail::worldHullNormals(hull, matHull);
        glm::vec3 hullCentroid = MeshCollisionDetail::centroidOf(hullVerts);

        std::vector<glm::vec3> axes = hullNormals;
        glm::vec3 segDir = segB - segA;
        if (glm::length(segDir) > 1e-6f) axes.push_back(glm::normalize(segDir));

        float bestOverlap = std::numeric_limits<float>::max();
        glm::vec3 bestAxis{0.0f};

        for (const glm::vec3& axis: axes) {
            float len = glm::length(axis);
            if (len < 1e-6f) continue;
            glm::vec3 a = axis / len;

            float projA = glm::dot(segA, a);
            float projB = glm::dot(segB, a);
            float capMin = std::min(projA, projB) - r;
            float capMax = std::max(projA, projB) + r;

            auto [hullMin, hullMax] = MeshCollisionDetail::projectPoints(hullVerts, a);

            float overlap = std::min(capMax, hullMax) - std::max(capMin, hullMin);
            if (overlap <= 0.0f) return std::nullopt;
            if (overlap < bestOverlap) {
                bestOverlap = overlap;
                float capCenterProj = (projA + projB) * 0.5f;
                float sign = (capCenterProj - glm::dot(hullCentroid, a) >= 0.0f) ? 1.0f : -1.0f;
                bestAxis = a * sign;
            }
        }

        if (bestOverlap == std::numeric_limits<float>::max()) return std::nullopt;
        return bestAxis * bestOverlap;
    }
} // namespace Physics
