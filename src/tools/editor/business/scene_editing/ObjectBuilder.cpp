#include "ObjectBuilder.hpp"
#include "../../../../engine/modules/scene/components/Renderer.hpp"
#include "../../../../engine/modules/scene/components/Transform.hpp"

nlohmann::json ObjectBuilder::cube() {
    std::string meshName = meshMutations_.generateCube();
    return makePrimitive(meshName, "Cube");
}

nlohmann::json ObjectBuilder::plane() {
    std::string meshName = meshMutations_.generatePlane();
    return makePrimitive(meshName, "Plane");
}

nlohmann::json ObjectBuilder::sphere(uint32_t resolution) {
    std::string meshName = meshMutations_.generateSphere(resolution);
    return makePrimitive(meshName, "Sphere");
}

nlohmann::json ObjectBuilder::makePrimitive(const std::string& meshName, const std::string& objectName) {
    Transform t{glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::vec3(1.0f)};
    Renderer r{meshName, SOLID_COLOR_MATERIAL, std::nullopt, true};
    return {
            {"metadata", {{"name", objectName}}},
            {"transform", t.serialize()},
            {"renderer", r.serialize()},
    };
}
