#pragma once
#include <glm/glm.hpp>
#include "../../../utils/JsonUtils.hpp"

struct BoxCollider {
    glm::vec3 center{0.0f};
    glm::vec3 halfExtents{0.5f};

    nlohmann::json serialize() const {
        return {{"center", JsonUtils::serializeVec3(center)}, {"halfExtents", JsonUtils::serializeVec3(halfExtents)}};
    }

    static BoxCollider deserialize(const nlohmann::json& j) {
        BoxCollider b{};
        b.center = JsonUtils::deserializeVec3(j.at("center"));
        b.halfExtents = JsonUtils::deserializeVec3(j.at("halfExtents"));
        return b;
    }
};
