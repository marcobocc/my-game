#pragma once
#include <filesystem>
#include <fstream>
#include <log4cxx/logger.h>
#include <memory>
#include <nlohmann/json.hpp>
#include "AssetCache.hpp"
#include "AssetExplorer.hpp"
#include "MeshImporter.hpp"
#include "asset_resources/MeshResource.hpp"
#include "asset_resources/ShaderResource.hpp"
#include "asset_resources/TextureResource.hpp"
#include "modules/asset_management/ProceduralMeshGenerator.hpp"
#include "modules/asset_management/asset_types/Material.hpp"
#include "modules/asset_management/asset_types/Model.hpp"
#include "stb_image.h"
#include "utils/JsonUtils.hpp"

class AssetLoader {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetLoader");

public:
    explicit AssetLoader(AssetExplorer& explorer, AssetCache& cache) : explorer_(explorer), cache_(cache) {}

    template<typename T>
    T* get(const std::string& name) {
        if (!cache_.contains(name)) import <T>(name);
        if (!cache_.contains(name)) return nullptr;
        return cache_.get<T>(name);
    }

private:
    template<typename T>
    void import(const std::string& name) const;

    AssetExplorer& explorer_;
    AssetCache& cache_;
};

template<>
inline void AssetLoader::import<MeshResource>(const std::string& name) const {
    if (name.starts_with("_PRIMITIVE")) {
        try {
            auto mesh = ProceduralMeshGenerator::dispatch(name);
            if (mesh) cache_.insert<MeshResource>(name, std::move(mesh));
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, "Failed to generate procedural mesh: " << name << " - " << e.what());
        }
    } else {
        auto absolutePath = explorer_.getAbsolutePath(name);
        auto mesh = importing::importObjFile(absolutePath, false, std::filesystem::path(name).stem().string());
        if (mesh) cache_.insert<MeshResource>(name, std::move(mesh));
    }
}

template<>
inline void AssetLoader::import<ShaderResource>(const std::string& name) const {
    try {
        auto absolutePath = explorer_.getAbsolutePath(name);
        auto j = JsonUtils::loadJson(absolutePath);
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

        auto readBytecode = [&](const std::string& path) -> std::vector<char> {
            auto absoluteBytecodeP = absolutePath.parent_path() / std::filesystem::path(path).filename();
            std::ifstream f(absoluteBytecodeP, std::ios::binary | std::ios::ate);
            if (!f) throw std::runtime_error("Failed to open shader bytecode: " + absoluteBytecodeP.string());
            std::streamsize size = f.tellg();
            f.seekg(0, std::ios::beg);
            std::vector<char> buf(size);
            if (!f.read(buf.data(), size))
                throw std::runtime_error("Failed to read shader bytecode: " + absoluteBytecodeP.string());
            return buf;
        };

        auto vertexBytecode = readBytecode(vertexShaderPath);
        auto fragmentBytecode = readBytecode(fragmentShaderPath);
        auto shader = std::make_unique<ShaderResource>(name,
                                                       std::move(vertexBytecode),
                                                       std::move(fragmentBytecode),
                                                       disableCull,
                                                       disableDepthTest,
                                                       disableDepthWrite,
                                                       enableAlphaBlend,
                                                       noVertexInput,
                                                       lineTopology,
                                                       positionColorVertexLayout,
                                                       tangentVertexLayout);
        if (shader) cache_.insert<ShaderResource>(name, std::move(shader));
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to import shader: " << name << " - " << e.what());
    }
}

template<>
inline void AssetLoader::import<TextureResource>(const std::string& name) const {
    auto absolutePath = explorer_.getAbsolutePath(name);
    if (absolutePath.empty() || !std::filesystem::exists(absolutePath)) {
        LOG4CXX_ERROR(LOGGER, "Texture file not found: " << absolutePath);
        return;
    }
    int width, height, channels;
    unsigned char* data = stbi_load(absolutePath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        LOG4CXX_ERROR(LOGGER, "Failed to decode texture: " << absolutePath << " - " << stbi_failure_reason());
        return;
    }
    constexpr int forcedChannels = 4;
    std::vector<unsigned char> imageData(data, data + width * height * forcedChannels);
    stbi_image_free(data);
    auto texture = std::make_unique<TextureResource>(name, width, height, forcedChannels, std::move(imageData));
    if (texture) cache_.insert<TextureResource>(name, std::move(texture));
}

template<>
inline void AssetLoader::import<Material>(const std::string& name) const {
    try {
        auto absolutePath = explorer_.getAbsolutePath(name);
        auto j = JsonUtils::loadJson(absolutePath);
        auto material = std::make_unique<Material>(Material::deserialize(j, absolutePath));
        if (material) cache_.insert<Material>(name, std::move(material));
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to import material: " << name << " - " << e.what());
    }
}

template<>
inline void AssetLoader::import<Model>(const std::string& name) const {
    try {
        auto absolutePath = explorer_.getAbsolutePath(name);
        auto j = JsonUtils::loadJson(absolutePath);
        Model def = Model::deserialize(j, absolutePath);
        auto model = std::make_unique<Model>(std::move(def));
        if (model) cache_.insert<Model>(name, std::move(model));
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to import model: " << name << " - " << e.what());
    }
}
