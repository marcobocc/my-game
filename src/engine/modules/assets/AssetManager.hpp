#pragma once
#include <string>
#include <type_traits>
#include <vector>
#include "AssetImporter.hpp"
#include "AssetStorage.hpp"
#include "ProceduralMeshGenerator.hpp"
#include "structs/assets/Mesh.hpp"

class AssetManager {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetManager");

public:
    explicit AssetManager(AssetImporter& importer, AssetStorage& cache) : assetImporter_(importer), storage_(cache) {}

    template<typename T>
    T* get(const std::string& name) {
        if (auto* cached = storage_.get<T>(name)) return cached;
        if constexpr (std::is_same_v<T, Mesh>) {
            if (name.starts_with("_PRIMITIVE")) {
                auto mesh = ProceduralMeshGenerator::generate(name);
                return storage_.insert(name, std::move(mesh));
            }
        }

        bool importSuccessful = assetImporter_.import(name);
        if (!importSuccessful) LOG4CXX_WARN(LOGGER, "Failed to import asset: " << name);
        return storage_.get<T>(name);
    }

    std::vector<std::string> getAvailableAssets(const std::string& extension) const {
        return assetImporter_.getAvailableAssets(extension);
    }

    void registerAsset(const std::string& relativePath) const { assetImporter_.registerAsset(relativePath); }

    std::filesystem::path getAbsolutePath(const std::string& relativePath) const {
        return assetImporter_.getAbsolutePath(relativePath);
    }

    bool isBuiltin(const std::string& relativePath) const { return assetImporter_.isBuiltin(relativePath); }

    template<typename T>
    void unload(const std::string& name) const {
        assetImporter_.deregisterAsset(name);
        storage_.remove<T>(name);
    }

    template<typename T>
    T* reload(const std::string& name) {
        storage_.remove<T>(name);
        return get<T>(name);
    }

private:
    AssetImporter& assetImporter_;
    AssetStorage& storage_;
};
