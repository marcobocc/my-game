#include "AssetBaker.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include "../../../../engine/modules/asset_management/asset_resources/MeshResource.hpp"
#include "../../../../engine/modules/asset_management/asset_types/Material.hpp"
#include "../../../../engine/utils/JsonUtils.hpp"

void AssetBaker::ensureDirectory(const std::filesystem::path& path) {
    std::filesystem::path dir = path.parent_path();
    if (!dir.empty() && !std::filesystem::exists(dir)) {
        std::filesystem::create_directories(dir);
    }
}

template<>
void AssetBaker::bake<Material>(const Material& material,
                                const std::string& assetName,
                                const std::filesystem::path& outputPath) {
    ensureDirectory(outputPath);
    std::ofstream file(outputPath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open material file for writing: " + outputPath.string());
    }
    file << material.serialize().dump(4);
    LOG4CXX_INFO(LOGGER, "Baked material: " << assetName);
}

template<>
void AssetBaker::bake<MeshResource>(const MeshResource& mesh,
                                    const std::string& assetName,
                                    const std::filesystem::path& outputPath) {
    ensureDirectory(outputPath);

    std::filesystem::path objPath = outputPath;
    objPath.replace_extension(".obj");
    std::ofstream objFile(objPath);
    if (!objFile.is_open()) {
        throw std::runtime_error("Failed to open mesh OBJ file for writing: " + objPath.string());
    }

    const auto& positions = mesh.getPositions();
    const auto& uvs = mesh.getUvs();
    const auto& normals = mesh.getNormals();
    const auto& indices = mesh.getIndices();

    for (const auto& pos: positions) {
        objFile << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
    }
    for (const auto& uv: uvs) {
        objFile << "vt " << uv.x << " " << uv.y << "\n";
    }
    for (const auto& normal: normals) {
        objFile << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
    }
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i] + 1;
        uint32_t i1 = indices[i + 1] + 1;
        uint32_t i2 = indices[i + 2] + 1;

        if (!uvs.empty() && !normals.empty()) {
            objFile << "f " << i0 << "/" << i0 << "/" << i0 << " " << i1 << "/" << i1 << "/" << i1 << " " << i2 << "/"
                    << i2 << "/" << i2 << "\n";
        } else {
            objFile << "f " << i0 << " " << i1 << " " << i2 << "\n";
        }
    }

    objFile.close();
    std::filesystem::path meshPath = outputPath;
    meshPath.replace_extension(".mesh");
    std::ofstream meshFile(meshPath);
    if (!meshFile.is_open()) {
        throw std::runtime_error("Failed to open mesh metadata file for writing: " + meshPath.string());
    }

    nlohmann::json meshMetadata;
    meshMetadata["meshFile"] = objPath.filename().string();
    meshMetadata["ccw"] = true;
    meshFile << meshMetadata.dump(4);
    meshFile.close();

    LOG4CXX_INFO(LOGGER, "Baked mesh: " << assetName);
}

template<typename T>
void AssetBaker::bake(const T& asset, const std::string& assetName, const std::filesystem::path& outputPath) {
    static_assert(false, "AssetBaker::bake<T> is not specialized for this type");
}
