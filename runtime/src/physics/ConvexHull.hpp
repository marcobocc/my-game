#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <limits>
#include <set>
#include <unordered_map>
#include <vector>

// Convex hull of a point set: the vertices actually on the hull (a subset of the
// input, deduplicated), triangulated faces with outward-facing normals, and the
// unique undirected edges (derived from the faces).
struct ConvexHull {
    std::vector<glm::vec3> vertices;
    std::vector<std::array<uint32_t, 3>> faces;
    std::vector<glm::vec3> faceNormals; // one per face, unit length, outward
    std::vector<std::pair<uint32_t, uint32_t>> edges;
};

namespace Physics {

    namespace ConvexHullDetail {
        constexpr float kEpsilon = 1e-5f;
        constexpr float kWeldQuantum = 1e-4f;

        inline std::vector<glm::vec3> weldPoints(const std::vector<glm::vec3>& points) {
            auto quantize = [](float v) { return static_cast<int64_t>(std::lround(v / kWeldQuantum)); };
            std::set<std::array<int64_t, 3>> seen;
            std::vector<glm::vec3> unique;
            unique.reserve(points.size());
            for (const auto& p: points) {
                std::array<int64_t, 3> key{quantize(p.x), quantize(p.y), quantize(p.z)};
                if (seen.insert(key).second) unique.push_back(p);
            }
            return unique;
        }

        // Recomputes face normals (flipping any face whose normal points toward the
        // hull's centroid) and derives the unique edge list. Safe to call on any
        // triangle soup, regardless of input winding.
        inline void finalize(ConvexHull& hull) {
            glm::vec3 centroid(0.0f);
            for (const auto& v: hull.vertices)
                centroid += v;
            if (!hull.vertices.empty()) centroid /= static_cast<float>(hull.vertices.size());

            hull.faceNormals.resize(hull.faces.size());
            for (size_t i = 0; i < hull.faces.size(); ++i) {
                auto& f = hull.faces[i];
                glm::vec3 pa = hull.vertices[f[0]], pb = hull.vertices[f[1]], pc = hull.vertices[f[2]];
                glm::vec3 n = glm::cross(pb - pa, pc - pa);
                float len = glm::length(n);
                n = len > 1e-8f ? n / len : glm::vec3(0.0f, 1.0f, 0.0f);
                if (glm::dot(n, pa - centroid) < 0.0f) {
                    std::swap(f[1], f[2]);
                    n = -n;
                }
                hull.faceNormals[i] = n;
            }

            std::set<std::pair<uint32_t, uint32_t>> edgeSet;
            for (const auto& f: hull.faces) {
                for (int i = 0; i < 3; ++i) {
                    uint32_t a = f[i], b = f[(i + 1) % 3];
                    edgeSet.insert(a < b ? std::make_pair(a, b) : std::make_pair(b, a));
                }
            }
            hull.edges.assign(edgeSet.begin(), edgeSet.end());
        }

        // Fallback for degenerate inputs (fewer than 4 points, or all points
        // collinear/coplanar): a small box enclosing the points, so callers always get
        // a valid 3D convex volume to work with.
        inline ConvexHull boxFallback(const std::vector<glm::vec3>& points) {
            glm::vec3 mn(std::numeric_limits<float>::max());
            glm::vec3 mx(std::numeric_limits<float>::lowest());
            for (const auto& p: points) {
                mn = glm::min(mn, p);
                mx = glm::max(mx, p);
            }
            if (points.empty()) mn = mx = glm::vec3(0.0f);
            constexpr float kMinExtent = 1e-3f;
            for (int i = 0; i < 3; ++i) {
                if (mx[i] - mn[i] < kMinExtent) {
                    mn[i] -= kMinExtent * 0.5f;
                    mx[i] += kMinExtent * 0.5f;
                }
            }

            ConvexHull hull;
            hull.vertices = {
                    {mn.x, mn.y, mn.z},
                    {mx.x, mn.y, mn.z},
                    {mx.x, mx.y, mn.z},
                    {mn.x, mx.y, mn.z},
                    {mn.x, mn.y, mx.z},
                    {mx.x, mn.y, mx.z},
                    {mx.x, mx.y, mx.z},
                    {mn.x, mx.y, mx.z},
            };
            auto addQuad = [&](uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
                hull.faces.push_back({a, b, c});
                hull.faces.push_back({a, c, d});
            };
            addQuad(0, 1, 2, 3);
            addQuad(5, 4, 7, 6);
            addQuad(4, 0, 3, 7);
            addQuad(1, 5, 6, 2);
            addQuad(4, 5, 1, 0);
            addQuad(3, 2, 6, 7);
            finalize(hull);
            return hull;
        }
    } // namespace ConvexHullDetail

    // Computes the convex hull of a point set via incremental (QuickHull-style) face
    // insertion: build an initial tetrahedron from extreme points, then for every
    // remaining point outside the current hull, remove the faces it can see and
    // stitch new faces from the horizon to that point. Degenerate inputs (too few
    // points, collinear, or coplanar) fall back to a small enclosing box.
    inline ConvexHull computeConvexHull(const std::vector<glm::vec3>& inputPoints) {
        using namespace ConvexHullDetail;
        std::vector<glm::vec3> points = weldPoints(inputPoints);
        if (points.size() < 4) return boxFallback(points.empty() ? inputPoints : points);

        uint32_t i0 = 0;
        for (uint32_t i = 1; i < points.size(); ++i)
            if (points[i].x < points[i0].x) i0 = i;

        uint32_t i1 = i0;
        float best = -1.0f;
        for (uint32_t i = 0; i < points.size(); ++i) {
            glm::vec3 d = points[i] - points[i0];
            float dist2 = glm::dot(d, d);
            if (dist2 > best) {
                best = dist2;
                i1 = i;
            }
        }
        if (i1 == i0) return boxFallback(points);

        uint32_t i2 = i0;
        best = -1.0f;
        glm::vec3 dir = points[i1] - points[i0];
        for (uint32_t i = 0; i < points.size(); ++i) {
            glm::vec3 c = glm::cross(dir, points[i] - points[i0]);
            float dist2 = glm::dot(c, c);
            if (dist2 > best) {
                best = dist2;
                i2 = i;
            }
        }
        if (best < kEpsilon) return boxFallback(points);

        glm::vec3 planeNormal = glm::normalize(glm::cross(points[i1] - points[i0], points[i2] - points[i0]));
        uint32_t i3 = i0;
        best = -1.0f;
        for (uint32_t i = 0; i < points.size(); ++i) {
            float dist = std::abs(glm::dot(planeNormal, points[i] - points[i0]));
            if (dist > best) {
                best = dist;
                i3 = i;
            }
        }
        if (best < kEpsilon) return boxFallback(points);

        struct Face {
            std::array<uint32_t, 3> v;
            glm::vec3 n;
        };
        std::vector<Face> faces;
        auto makeFace = [&](uint32_t a, uint32_t b, uint32_t c, uint32_t opposite) {
            glm::vec3 n = glm::cross(points[b] - points[a], points[c] - points[a]);
            if (glm::dot(n, points[opposite] - points[a]) > 0.0f) {
                std::swap(b, c);
                n = -n;
            }
            float len = glm::length(n);
            faces.push_back({{a, b, c}, len > 1e-8f ? n / len : glm::vec3(0.0f, 1.0f, 0.0f)});
        };
        makeFace(i0, i1, i2, i3);
        makeFace(i0, i2, i3, i1);
        makeFace(i0, i3, i1, i2);
        makeFace(i1, i3, i2, i0);

        std::vector<bool> used(points.size(), false);
        used[i0] = used[i1] = used[i2] = used[i3] = true;

        auto edgeKey = [](uint32_t a, uint32_t b) -> uint64_t {
            uint32_t lo = std::min(a, b), hi = std::max(a, b);
            return (static_cast<uint64_t>(lo) << 32) | hi;
        };

        for (uint32_t p = 0; p < points.size(); ++p) {
            if (used[p]) continue;

            std::vector<size_t> visible;
            for (size_t f = 0; f < faces.size(); ++f)
                if (glm::dot(faces[f].n, points[p] - points[faces[f].v[0]]) > kEpsilon) visible.push_back(f);
            if (visible.empty()) continue;

            std::unordered_map<uint64_t, std::pair<uint32_t, uint32_t>> edgeOf;
            std::unordered_map<uint64_t, int> count;
            for (size_t f: visible) {
                const auto& v = faces[f].v;
                for (int e = 0; e < 3; ++e) {
                    uint32_t a = v[e], b = v[(e + 1) % 3];
                    uint64_t k = edgeKey(a, b);
                    count[k]++;
                    edgeOf[k] = {a, b};
                }
            }
            std::vector<std::pair<uint32_t, uint32_t>> horizon;
            for (const auto& [k, c]: count)
                if (c == 1) horizon.push_back(edgeOf[k]);

            std::sort(visible.begin(), visible.end(), std::greater<>());
            for (size_t f: visible)
                faces.erase(faces.begin() + static_cast<long>(f));

            for (const auto& [a, b]: horizon) {
                glm::vec3 n = glm::cross(points[b] - points[a], points[p] - points[a]);
                float len = glm::length(n);
                faces.push_back({{a, b, p}, len > 1e-8f ? n / len : glm::vec3(0.0f, 1.0f, 0.0f)});
            }

            used[p] = true;
        }

        ConvexHull hull;
        std::unordered_map<uint32_t, uint32_t> remap;
        hull.faces.reserve(faces.size());
        for (const auto& f: faces) {
            std::array<uint32_t, 3> tri{};
            for (int k = 0; k < 3; ++k) {
                uint32_t idx = f.v[k];
                auto it = remap.find(idx);
                if (it == remap.end()) {
                    uint32_t newIdx = static_cast<uint32_t>(hull.vertices.size());
                    hull.vertices.push_back(points[idx]);
                    remap[idx] = newIdx;
                    tri[k] = newIdx;
                } else {
                    tri[k] = it->second;
                }
            }
            hull.faces.push_back(tri);
        }
        finalize(hull);
        return hull;
    }

} // namespace Physics
