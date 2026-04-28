#include "AssetImporter.hpp"
#include <array>
#include <fstream>
#include <glm/glm.hpp>
#include <ranges>
#include <stdexcept>
#include "assets/AssetDescriptors.hpp"
#include "assets/AssetStorage.hpp"
#include "assets/types/Material.hpp"
#include "assets/types/Mesh.hpp"
#include "assets/types/Shader.hpp"
#include "assets/types/Texture.hpp"
#include "importing.hpp"
#include "stb_image.h"

AssetImporter::AssetImporter(const std::filesystem::path& root, AssetStorage& cache) : storage_(cache), root_(root) {
    searchAssets();
}

std::filesystem::path AssetImporter::toAbsolutePath(const std::filesystem::path& relativePath) const {
    if (relativePath.is_absolute()) return relativePath;
    return root_ / relativePath;
}

void AssetImporter::searchAssets() {
    static constexpr std::array knownExtensions = {".shad", ".mesh", ".tex", ".mat"};
    LOG4CXX_INFO(LOGGER, "Searching for assets in: " << root_);
    for (const auto& file: std::filesystem::recursive_directory_iterator(root_)) {
        auto extension = file.path().extension().string();
        if (std::ranges::find(knownExtensions, extension) == knownExtensions.end()) continue;
        auto name = std::filesystem::relative(file.path(), root_).string();
        auto absolutePath = toAbsolutePath(file.path());
        auto [it, inserted] = availableAssetFiles_.emplace(name, absolutePath);
        LOG4CXX_INFO(LOGGER, "Discovered asset: " << it->first);
    }
}

bool AssetImporter::import(const std::string& name) {
    auto it = availableAssetFiles_.find(name);
    if (it == availableAssetFiles_.end()) throw std::runtime_error("Asset not found: " + name);
    const auto& path = it->second;
    auto ext = path.extension().string();

    bool importSuccessful = false;
    if (ext == ".mesh") importSuccessful = importMesh(path, name);
    if (ext == ".shad") importSuccessful = importShader(path, name);
    if (ext == ".tex") importSuccessful = importTexture(path, name);
    if (ext == ".mat") importSuccessful = importMaterial(path, name);

    if (!importSuccessful) {
        LOG4CXX_WARN(LOGGER, "Failed to import asset: " << name);
        return false;
    }
    LOG4CXX_INFO(LOGGER, "Successfully imported asset from disk: " << name);
    return true;
}

bool AssetImporter::importMesh(const std::filesystem::path& file, const std::string& name) const {
    MeshDescriptor def = MeshDescriptor::fromFile(file, name);
    auto meshFilePath = toAbsolutePath(def.meshFile);
    if (meshFilePath.extension() == ".obj") {
        storage_.insert<Mesh>(name, importing::importObjFile(meshFilePath, !def.ccw, name));
        return true;
    }
    LOG4CXX_ERROR(LOGGER, "Unsupported mesh extension: " << meshFilePath.extension() << " in " << meshFilePath);
    return false;
}

bool AssetImporter::importShader(const std::filesystem::path& file, const std::string& name) const {
    ShaderDescriptor def = ShaderDescriptor::fromFile(file, name);
    auto readFile = [&](const std::filesystem::path& path) -> std::vector<char> {
        std::ifstream f(path, std::ios::binary | std::ios::ate);
        if (!f) throw std::runtime_error("Failed to open shader file: " + path.string());
        std::streamsize size = f.tellg();
        f.seekg(0, std::ios::beg);
        std::vector<char> buf(size);
        if (!f.read(buf.data(), size)) throw std::runtime_error("Failed to read shader file: " + path.string());
        return buf;
    };
    std::vector<char> vertexBytecode, fragmentBytecode;
    try {
        vertexBytecode = readFile(toAbsolutePath(def.vertexShader));
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, e.what());
        return false;
    }
    try {
        fragmentBytecode = readFile(toAbsolutePath(def.fragmentShader));
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, e.what());
        return false;
    }
    storage_.insert<Shader>(name,
                            std::make_unique<Shader>(name,
                                                     std::move(vertexBytecode),
                                                     std::move(fragmentBytecode),
                                                     def.disableCull,
                                                     def.disableDepthTest,
                                                     def.disableDepthWrite,
                                                     def.enableAlphaBlend,
                                                     def.noVertexInput,
                                                     def.lineTopology,
                                                     def.positionColorVertexLayout));
    return true;
}

bool AssetImporter::importTexture(const std::filesystem::path& file, const std::string& name) const {
    TextureDescriptor def = TextureDescriptor::fromFile(file, name);
    auto imageFilePath = toAbsolutePath(def.image);
    int w, h, c;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(imageFilePath.c_str(), &w, &h, &c, 4);
    if (!data) {
        LOG4CXX_ERROR(LOGGER, "Failed to load image file: " << imageFilePath);
        return false;
    }
    size_t size = w * h * 4;
    std::vector<unsigned char> buffer(data, data + size);
    stbi_image_free(data);
    storage_.insert<Texture>(name, std::make_unique<Texture>(name, w, h, 4, std::move(buffer)));
    return true;
}

bool AssetImporter::importMaterial(const std::filesystem::path& file, const std::string& name) const {
    MaterialDescriptor def = MaterialDescriptor::fromFile(file, name);
    storage_.insert<Material>(name, std::make_unique<Material>(name, def.shaderName, def.baseColor, def.textureName));
    return true;
}
