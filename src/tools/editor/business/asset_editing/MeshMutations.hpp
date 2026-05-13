#pragma once
#include <memory>
#include "../../../../engine/modules/asset_management/asset_resources/MeshResource.hpp"
#include "EditorAssetRepository.hpp"

class MeshMutations {
public:
    MeshMutations(EditorAssetRepository& repository) : repository_(repository) {}

    std::string generateSphere(uint32_t resolution);
    std::string generateCube();
    std::string generatePlane();

private:
    EditorAssetRepository& repository_;

    static std::unique_ptr<MeshResource> buildSphere(uint32_t resolution, const std::string& name);
    static std::unique_ptr<MeshResource> buildCube(const std::string& name);
    static std::unique_ptr<MeshResource> buildPlane(const std::string& name);
};
