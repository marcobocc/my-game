#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};

    glm::mat4 getModelMatrix() const {
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotate = glm::toMat4(rotation);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
        return translate * rotate * scaleMat;
    }
};
