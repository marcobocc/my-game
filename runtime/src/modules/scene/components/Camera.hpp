#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>
#include "../../../structs/RenderTargetHandle.hpp"
#include "IComponent.hpp"

struct Camera final : IComponent {
    float fov = 45.0f;
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    RenderTargetHandle renderTarget;

    glm::mat4 getProjectionMatrix() const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
        proj[1][1] *= -1.0f; // Vulkan NDC has Y pointing down; flip here so the Vulkan backend uses plain
                             // positive-height viewports
        return proj;
    }

    nlohmann::json serialize() const override {
        return {{"fov", fov}, {"aspect", aspect}, {"nearPlane", nearPlane}, {"farPlane", farPlane}};
    }

    std::string typeName() const override { return "Camera"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<Camera>(*this); }

    static Camera deserialize(const nlohmann::json& j) {
        Camera c{};
        c.fov = j.at("fov").get<float>();
        c.aspect = j.at("aspect").get<float>();
        c.nearPlane = j.at("nearPlane").get<float>();
        c.farPlane = j.at("farPlane").get<float>();
        return c;
    }
};
