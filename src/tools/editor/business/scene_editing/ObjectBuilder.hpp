#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "../../../../engine/modules/scene/components/Renderer.hpp"
#include "../../../../engine/modules/scene/components/Transform.hpp"
#include "../asset_editing/MeshMutations.hpp"

class ObjectBuilder {
public:
    ObjectBuilder(MeshMutations& meshMutations) : meshMutations_(meshMutations) {}

    nlohmann::json cube();
    nlohmann::json plane();
    nlohmann::json sphere(uint32_t resolution);

private:
    MeshMutations& meshMutations_;

    nlohmann::json makePrimitive(const std::string& meshName, const std::string& objectName);
};
