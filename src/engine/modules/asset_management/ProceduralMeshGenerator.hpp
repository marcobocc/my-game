#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include "../../modules/asset_management/asset_resources/MeshResource.hpp"

class ProceduralMeshGenerator {
public:
    static std::unique_ptr<MeshResource> dispatch(const std::string& name);

private:
    static std::unique_ptr<MeshResource> generateSphere(uint32_t resolution, const std::string& name);
    static std::unique_ptr<MeshResource> generateCube(const std::string& name);
    static std::unique_ptr<MeshResource> generatePlane(const std::string& name);
};
