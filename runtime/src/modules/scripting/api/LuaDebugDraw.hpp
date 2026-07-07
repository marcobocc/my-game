#pragma once
#include <glm/glm.hpp>
#include "modules/debug/DebugDraw.hpp"

class LuaDebugDraw {
public:
    explicit LuaDebugDraw(DebugDraw& debugDraw) : debugDraw_(debugDraw) {}

    void line(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) { debugDraw_.line(from, to, color); }

    void lineDefault(const glm::vec3& from, const glm::vec3& to) { debugDraw_.line(from, to, {0.0f, 1.0f, 0.0f}); }

    void box(const glm::vec3& center, const glm::vec3& halfExtents, const glm::vec3& color) {
        glm::mat4 identity(1.0f);
        debugDraw_.box(identity, halfExtents, center, color);
    }

    void boxDefault(const glm::vec3& center, const glm::vec3& halfExtents) {
        box(center, halfExtents, {0.0f, 1.0f, 0.5f});
    }

    void sphere(const glm::vec3& center, float radius, const glm::vec3& color) {
        BoundingSphere s;
        s.center = center;
        s.radius = radius;
        debugDraw_.sphere(s, color);
    }

    void sphereDefault(const glm::vec3& center, float radius) { sphere(center, radius, {1.0f, 1.0f, 0.0f}); }

private:
    DebugDraw& debugDraw_;
};
