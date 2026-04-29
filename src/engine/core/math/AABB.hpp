#pragma once
#include <glm/vec3.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    AABB() : min(glm::vec3(FLT_MAX)), max(glm::vec3(-FLT_MAX)) {}

    explicit AABB(const std::vector<glm::vec3>& positions) : AABB() {
        for (const auto& p: positions) {
            min = glm::min(min, p);
            max = glm::max(max, p);
        }
    }

    std::vector<glm::vec3> getCorners() const {
        return {{min.x, min.y, min.z},
                {max.x, min.y, min.z},
                {min.x, max.y, min.z},
                {max.x, max.y, min.z},
                {min.x, min.y, max.z},
                {max.x, min.y, max.z},
                {min.x, max.y, max.z},
                {max.x, max.y, max.z}};
    }

    glm::vec3 getCentroid() const {
        return (min + max) * 0.5f;
    }

    AABB applyTransform(const glm::mat4& transform) const {
        auto corners = getCorners();
        std::vector<glm::vec3> newCorners(corners.size());
        for (auto& c: corners) {
            newCorners.push_back(transform * glm::vec4(c, 1.0f));
        }
        return AABB(newCorners);
    }
};
