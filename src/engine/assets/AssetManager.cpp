#include "AssetManager.hpp"
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include "assets/types/AssetDescriptors.hpp"
#include "assets/types/material/MaterialLoader.hpp"
#include "assets/types/mesh/MeshDataLoader.hpp"
#include "assets/types/shader/ShaderLoader.hpp"
#include "assets/types/texture/TextureLoader.hpp"

AssetManager::AssetManager(AssetCache& assetCache, const std::filesystem::path& root) : cache_(assetCache) {
    registerLoader(std::make_unique<ShaderLoader>());
    registerLoader(std::make_unique<MeshDataLoader>());
    registerLoader(std::make_unique<TextureLoader>());
    registerLoader(std::make_unique<MaterialLoader>());
    scan(root);
}

void AssetManager::registerLoader(std::unique_ptr<AssetLoader> loader) {
    loaders_[loader->getAssetType()] = std::move(loader);
}

AssetManager::AssetInfo& AssetManager::getAssetInfo(const std::string& name) {
    auto assetIt = assetsInfos_.find(name);
    if (assetIt == assetsInfos_.end()) throw std::runtime_error("Asset not found: " + name);
    return assetIt->second;
}

void AssetManager::load(const std::string& name) { dispatchLoader(name); }

bool AssetManager::dispatchLoader(const std::string& name) {
    auto& assetInfo = getAssetInfo(name);
    auto loaderIt = loaders_.find(assetInfo.type);
    if (loaderIt == loaders_.end()) throw std::runtime_error("Asset loader not registered for type: " + assetInfo.type);
    loaderIt->second->load(assetInfo.descriptor, cache_);
    assetInfo.loaded = cache_.contains(name);
    return assetInfo.loaded;
}

std::vector<AssetManager::AssetInfo> AssetManager::getAssetInfos() const {
    std::vector<AssetInfo> infos;
    infos.reserve(assetsInfos_.size());
    for (const auto& info: assetsInfos_ | std::views::values) {
        infos.push_back(info);
    }
    return infos;
}

void AssetManager::scan(const std::filesystem::path& root) {
    LOG4CXX_INFO(LOGGER, "Scanning for assets: " << root);
    for (const auto& file: std::filesystem::recursive_directory_iterator(root)) {
        if (file.path().extension() != ".asset") continue;
        try {
            auto [name, type] = AssetDescriptor::fromFile(file.path());
            AssetInfo info{.name = name, .type = type, .descriptor = file.path(), .loaded = false};
            auto [it, inserted] = assetsInfos_.emplace(info.name, std::move(info));
            if (!inserted) {
                LOG4CXX_ERROR(LOGGER, "Duplicate asset: " << name << " in " << file.path());
                continue;
            }
            LOG4CXX_INFO(LOGGER, "Found " << type << ": " << name);
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, "Invalid asset info: " << file.path() << ", error: " << e.what());
        }
    }
}
