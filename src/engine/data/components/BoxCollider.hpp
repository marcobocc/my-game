#pragma once
#include <glm/glm.hpp>

struct BoxCollider {
    glm::vec3 center{0.0f}; // local-space offset from transform position
    glm::vec3 halfExtents{0.5f};
};
