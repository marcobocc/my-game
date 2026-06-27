#pragma once
#include <glm/glm.hpp>
#include "World.hpp"
#include "components/Transform.hpp"

namespace TransformUtils {

    inline glm::mat4 resolveWorldMatrix(const Transform& t, const World& world) {
        if (t.parent == INVALID_ENTITY_HANDLE) return t.getModelMatrix();
        const Actor* parentActor = world.getActor(t.parent);
        if (!parentActor) return t.getModelMatrix();
        const Transform* parentTransform = parentActor->getComponent<Transform>();
        if (!parentTransform) return t.getModelMatrix();
        return resolveWorldMatrix(*parentTransform, world) * t.getModelMatrix();
    }

} // namespace TransformUtils
