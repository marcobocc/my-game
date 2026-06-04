#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <memory>
#include <nlohmann/json.hpp>
#include "AssetCache.hpp"
#include "MeshImporter.hpp"
#include "VirtualFileSystem.hpp"
#include "asset_types/Mesh.hpp"
#include "asset_types/Shader.hpp"
#include "asset_types/Texture.hpp"
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/asset_management/asset_types/Model.hpp"
#include "stb_image.h"
#include "utils/JsonUtils.hpp"

class AssetLoader {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetLoader");

public:
    explicit AssetLoader(VirtualFileSystem& vfs, AssetCache& cache) : vfs_(vfs), cache_(cache) {}

    template<typename T>
    T* get(const std::string& name) {
        if (!cache_.contains(name)) load<T>(name);
        if (!cache_.contains(name)) return nullptr;
        return cache_.get<T>(name);
    }

private:
    template<typename T>
    std::unique_ptr<T> load(const std::string& name) const;

    VirtualFileSystem& vfs_;
    AssetCache& cache_;
};

template<>
inline std::unique_ptr<Mesh> AssetLoader::load<Mesh>(const std::string& name) const {
    if (!vfs_.exists(name)) {
        LOG4CXX_ERROR(LOGGER, "Asset not found: " << name);
        return nullptr;
    }
    std::string objFilePath = name;
    bool reverseWinding = false;
    if (std::filesystem::path(name).extension() == ".mesh") {
        try {
            auto meshData = vfs_.read(name);
            auto meshStr = std::string(meshData.begin(), meshData.end());
            auto j = nlohmann::json::parse(meshStr);
            objFilePath = JsonUtils::getRequired<std::string>(j, "meshFile");
            bool ccw = JsonUtils::getOptional<bool>(j, "ccw", true);
            reverseWinding = !ccw;
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, "Failed to read mesh metadata: " << name << " - " << e.what());
            return nullptr;
        }
    }
    auto mesh = importing::importObjFile(objFilePath, reverseWinding, name, vfs_);
    if (mesh) {
        auto meshPtr = mesh.get();
        cache_.insert<Mesh>(std::move(mesh));
        LOG4CXX_INFO(LOGGER, "Successfully loaded mesh: " << name);
        return std::make_unique<Mesh>(*meshPtr);
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Shader> AssetLoader::load<Shader>(const std::string& name) const {
    try {
        auto shaderDataVec = vfs_.read(name);
        auto shaderStr = std::string(shaderDataVec.begin(), shaderDataVec.end());
        auto j = nlohmann::json::parse(shaderStr);

        std::string vertexShaderPath = JsonUtils::getRequired<std::string>(j, "vertexShader");
        std::string fragmentShaderPath = JsonUtils::getRequired<std::string>(j, "fragmentShader");
        bool disableCull = JsonUtils::getOptional<bool>(j, "disableCull", false);
        bool disableDepthTest = JsonUtils::getOptional<bool>(j, "disableDepthTest", false);
        bool disableDepthWrite = JsonUtils::getOptional<bool>(j, "disableDepthWrite", false);
        bool enableAlphaBlend = JsonUtils::getOptional<bool>(j, "enableAlphaBlend", false);
        bool noVertexInput = JsonUtils::getOptional<bool>(j, "noVertexInput", false);
        bool lineTopology = JsonUtils::getOptional<bool>(j, "lineTopology", false);
        bool positionColorVertexLayout = JsonUtils::getOptional<bool>(j, "positionColorVertexLayout", false);
        bool tangentVertexLayout = JsonUtils::getOptional<bool>(j, "tangentVertexLayout", false);
        bool depthBias = JsonUtils::getOptional<bool>(j, "depthBias", false);

        auto readBytecode = [&](const std::string& path) -> std::vector<char> {
            auto data = vfs_.read(path);
            return std::vector<char>(data.begin(), data.end());
        };

        auto vertexBytecode = readBytecode(vertexShaderPath);
        auto fragmentBytecode = readBytecode(fragmentShaderPath);
        auto shader = std::make_unique<Shader>(name,
                                               std::move(vertexBytecode),
                                               std::move(fragmentBytecode),
                                               disableCull,
                                               disableDepthTest,
                                               disableDepthWrite,
                                               enableAlphaBlend,
                                               noVertexInput,
                                               lineTopology,
                                               positionColorVertexLayout,
                                               tangentVertexLayout,
                                               depthBias);
        if (shader) {
            auto shaderPtr = shader.get();
            cache_.insert<Shader>(std::move(shader));
            LOG4CXX_INFO(LOGGER, "Successfully loaded shader: " << name);
            return std::make_unique<Shader>(*shaderPtr);
        }
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load shader: " << name << " - " << e.what());
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Texture> AssetLoader::load<Texture>(const std::string& name) const {
    if (!vfs_.exists(name)) {
        LOG4CXX_ERROR(LOGGER, "Texture file not found in VFS: " << name);
        return nullptr;
    }
    auto imageData = vfs_.read(name);
    if (imageData.empty()) {
        LOG4CXX_ERROR(LOGGER, "Failed to read texture data: " << name);
        return nullptr;
    }
    int width, height, channels;
    unsigned char* data =
            stbi_load_from_memory(imageData.data(), imageData.size(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        LOG4CXX_ERROR(LOGGER, "Failed to decode texture: " << name << " - " << stbi_failure_reason());
        return nullptr;
    }
    constexpr int forcedChannels = 4;
    std::vector<unsigned char> decodedData(data, data + width * height * forcedChannels);
    stbi_image_free(data);
    auto texture = std::make_unique<Texture>(name, width, height, forcedChannels, std::move(decodedData));
    if (texture) {
        auto texturePtr = texture.get();
        cache_.insert<Texture>(std::move(texture));
        LOG4CXX_INFO(LOGGER, "Successfully loaded texture: " << name);
        return std::make_unique<Texture>(*texturePtr);
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Material> AssetLoader::load<Material>(const std::string& name) const {
    try {
        auto materialDataVec = vfs_.read(name);
        auto materialStr = std::string(materialDataVec.begin(), materialDataVec.end());
        auto j = nlohmann::json::parse(materialStr);
        auto material = std::make_unique<Material>(Material::deserialize(j, name));
        if (material) {
            auto materialPtr = material.get();
            cache_.insert<Material>(std::move(material));
            LOG4CXX_INFO(LOGGER, "Successfully loaded material: " << name);
            return std::make_unique<Material>(*materialPtr);
        }
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load material: " << name << " - " << e.what());
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Model> AssetLoader::load<Model>(const std::string& name) const {
    try {
        auto modelDataVec = vfs_.read(name);
        auto modelStr = std::string(modelDataVec.begin(), modelDataVec.end());
        auto j = nlohmann::json::parse(modelStr);
        Model def = Model::deserialize(j, name);
        auto model = std::make_unique<Model>(std::move(def));
        if (model) {
            auto modelPtr = model.get();
            cache_.insert<Model>(std::move(model));
            LOG4CXX_INFO(LOGGER, "Successfully loaded model: " << name);
            return std::make_unique<Model>(*modelPtr);
        }
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load model: " << name << " - " << e.what());
    }
    return nullptr;
}
