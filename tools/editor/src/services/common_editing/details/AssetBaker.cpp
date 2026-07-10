#include "AssetBaker.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <sstream>
#include "../../../../../../runtime/src/graphics/assets/Material.hpp"
#include "graphics/assets/Mesh.hpp"

template<>
void AssetBaker::bake<Material>(const Material& material, const std::string& assetName) {
    std::string serialized = material.serialize().dump(4);
    std::vector<unsigned char> data(serialized.begin(), serialized.end());
    if (!vfs_.write(assetName, data)) {
        throw std::runtime_error("Failed to write material: " + assetName);
    }
    LOG4CXX_INFO(LOGGER, "Baked material: " << assetName);
}

template<>
void AssetBaker::bake<Mesh>(const Mesh& mesh, const std::string& assetName) {
    const auto& positions = mesh.getPositions();
    const auto& uvs = mesh.getUvs();
    const auto& normals = mesh.getNormals();
    const auto& indices = mesh.getIndices();

    std::ostringstream obj;
    for (const auto& pos: positions)
        obj << "v " << pos.x << " " << pos.y << " " << pos.z << "\n";
    for (const auto& uv: uvs)
        obj << "vt " << uv.x << " " << uv.y << "\n";
    for (const auto& normal: normals)
        obj << "vn " << normal.x << " " << normal.y << " " << normal.z << "\n";
    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t i0 = indices[i] + 1;
        uint32_t i1 = indices[i + 1] + 1;
        uint32_t i2 = indices[i + 2] + 1;
        if (!uvs.empty() && !normals.empty()) {
            obj << "f " << i0 << "/" << i0 << "/" << i0 << " " << i1 << "/" << i1 << "/" << i1 << " " << i2 << "/" << i2
                << "/" << i2 << "\n";
        } else {
            obj << "f " << i0 << " " << i1 << " " << i2 << "\n";
        }
    }

    std::string objPath = std::filesystem::path(assetName).replace_extension(".obj").string();
    std::string objStr = obj.str();
    if (!vfs_.write(objPath, std::vector<unsigned char>(objStr.begin(), objStr.end()))) {
        throw std::runtime_error("Failed to write OBJ: " + objPath);
    }

    nlohmann::json meta;
    meta["meshFile"] = objPath;
    meta["ccw"] = true;
    std::string metaStr = meta.dump(4);
    if (!vfs_.write(assetName, std::vector<unsigned char>(metaStr.begin(), metaStr.end()))) {
        throw std::runtime_error("Failed to write mesh metadata: " + assetName);
    }

    LOG4CXX_INFO(LOGGER, "Baked mesh: " << assetName);
}

template<typename T>
void AssetBaker::bake(const T&, const std::string&) {
    static_assert(false, "AssetBaker::bake<T> is not specialized for this type");
}
