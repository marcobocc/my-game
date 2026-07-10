#pragma once
#include <glm/glm.hpp>
#include "../../core/components/IComponent.hpp"
#include "../../utils/JsonUtils.hpp"

struct CapsuleCollider final : IComponent {
    glm::vec3 center{0.0f};
    float radius{0.5f};
    float height{1.0f}; // distance between the two hemisphere centers (not total height)

    nlohmann::json serialize() const override {
        return {{"center", JsonUtils::serializeVec3(center)}, {"radius", radius}, {"height", height}};
    }

    std::string typeName() const override { return "CapsuleCollider"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<CapsuleCollider>(*this); }

    static CapsuleCollider deserialize(const nlohmann::json& j) {
        CapsuleCollider c{};
        c.center = JsonUtils::deserializeVec3(j.at("center"));
        c.radius = j.at("radius").get<float>();
        c.height = j.at("height").get<float>();
        return c;
    }
};
