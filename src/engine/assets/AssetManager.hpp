#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <unordered_map>
#include <vector>
#include "AssetCache.hpp"
#include "types/base/AssetLoader.hpp"

class AssetCache;
class AssetLoader;

class AssetManager {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetManager");

public:
    struct AssetInfo {
        std::string name;
        std::string type;
        std::filesystem::path descriptor;
        bool loaded;
    };

    explicit AssetManager(AssetCache& assetCache, const std::filesystem::path& root);

    template<typename T>
    T* get(const std::string& name);
    void load(const std::string& name);

    std::vector<AssetInfo> getAssetInfos() const;

private:
    AssetInfo& getAssetInfo(const std::string& name);
    void registerLoader(std::unique_ptr<AssetLoader> loader);
    bool dispatchLoader(const std::string& name);
    void scan(const std::filesystem::path& root);

    AssetCache& cache_;
    std::unordered_map<std::string, AssetInfo> assetsInfos_;
    std::unordered_map<std::string, std::unique_ptr<AssetLoader>> loaders_;
};

// ----------------------------------------------------
// Template definitions
// ----------------------------------------------------

template<typename T>
T* AssetManager::get(const std::string& name) {
    if (auto* cached = cache_.get<T>(name)) return cached;
    load(name);
    return cache_.get<T>(name);
}
