#pragma once
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <vector>

struct BoundingSphere {
    glm::vec3 center;
    float radius;

    BoundingSphere() : center(0.0f), radius(0.0f) {}

    explicit BoundingSphere(const std::vector<glm::vec3>& positions) : BoundingSphere() {
        if (positions.empty()) return;

        glm::vec3 centroid(0.0f);
        for (const auto& p: positions) {
            centroid += p;
        }
        centroid /= static_cast<float>(positions.size());
        center = centroid;

        radius = 0.0f;
        for (const auto& p: positions) {
            float dist = glm::distance(center, p);
            if (dist > radius) {
                radius = dist;
            }
        }
    }

    BoundingSphere applyTransform(const glm::mat4& transform) const {
        glm::vec3 newCenter = transform * glm::vec4(center, 1.0f);
        glm::vec3 point = center + glm::vec3(radius, 0.0f, 0.0f);
        glm::vec3 newPoint = transform * glm::vec4(point, 1.0f);
        float newRadius = glm::distance(newCenter, newPoint);

        BoundingSphere result;
        result.center = newCenter;
        result.radius = newRadius;
        return result;
    }
};
