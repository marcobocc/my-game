#pragma once
#include <glm/glm.hpp>
#include "../../core/components/IComponent.hpp"
#include "../../utils/JsonUtils.hpp"

struct BoxCollider final : IComponent {
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{0.5f};

    nlohmann::json serialize() const override {
        return {{"center", JsonUtils::serializeVec3(center)}, {"halfExtents", JsonUtils::serializeVec3(halfExtents)}};
    }

    std::string typeName() const override { return "BoxCollider"; }
    std::unique_ptr<IComponent> clone() const override { return std::make_unique<BoxCollider>(*this); }

    static BoxCollider deserialize(const nlohmann::json& j) {
        BoxCollider b{};
        b.center = JsonUtils::deserializeVec3(j.at("center"));
        b.halfExtents = JsonUtils::deserializeVec3(j.at("halfExtents"));
        return b;
    }
};
