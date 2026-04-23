#include "AssetManager.hpp"
#include <stdexcept>
#include "assets/types/material/MaterialLoader.hpp"
#include "assets/types/mesh/MeshLoader.hpp"
#include "assets/types/shader/ShaderLoader.hpp"
#include "assets/types/texture/TextureLoader.hpp"

AssetManager::AssetManager(AssetCache& assetCache, const std::filesystem::path& root) : cache_(assetCache) {
    registerLoader(".shad", std::make_unique<ShaderLoader>());
    registerLoader(".mesh", std::make_unique<MeshLoader>());
    registerLoader(".tex", std::make_unique<TextureLoader>());
    registerLoader(".mat", std::make_unique<MaterialLoader>());
    scan(root);
}

void AssetManager::registerLoader(const std::string& extension, std::unique_ptr<AssetLoader> loader) {
    loaders_[extension] = std::move(loader);
}

AssetManager::AssetInfo& AssetManager::getAssetInfo(const std::string& name) {
    auto it = assetsInfos_.find(name);
    if (it == assetsInfos_.end()) throw std::runtime_error("Asset not found: " + name);
    return it->second;
}

void AssetManager::load(const std::string& name) { dispatchLoader(name); }

bool AssetManager::dispatchLoader(const std::string& name) {
    auto& assetInfo = getAssetInfo(name);
    auto extension = assetInfo.descriptor.extension().string();
    auto loaderIt = loaders_.find(extension);
    if (loaderIt == loaders_.end()) throw std::runtime_error("No loader for extension: " + extension);
    loaderIt->second->load(assetInfo.descriptor, name, cache_);
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
        auto extension = file.path().extension().string();
        if (!loaders_.contains(extension)) continue;
        auto name = file.path().filename().string();
        AssetInfo info{.name = name, .descriptor = file.path(), .loaded = false};
        auto [it, inserted] = assetsInfos_.emplace(name, std::move(info));
        if (!inserted) {
            LOG4CXX_ERROR(LOGGER, "Duplicate asset: " << name << " in " << file.path());
            continue;
        }
        LOG4CXX_INFO(LOGGER, "Found asset: " << name);
    }
}
