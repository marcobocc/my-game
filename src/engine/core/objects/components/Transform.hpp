#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4 getModelMatrix() const {
        glm::mat4 translate = glm::translate(glm::mat4(1.0f), position);
        glm::mat4 rotate = glm::toMat4(rotation);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), scale);
        return translate * rotate * scaleMat;
    }

    glm::vec3 getForward() const { return glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f)); }

    glm::vec3 getUp() const { return glm::normalize(rotation * glm::vec3(0.0f, 1.0f, 0.0f)); }

    glm::vec3 getLookAt() const { return position + getForward(); }

    glm::mat4 getViewMatrix() const { return glm::lookAt(position, getLookAt(), getUp()); }
};
