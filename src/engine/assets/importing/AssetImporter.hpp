#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <unordered_set>

class AssetStorage;

class AssetImporter {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetImporter");

public:
    explicit AssetImporter(const std::filesystem::path& root, AssetStorage& cache);
    bool import(const std::filesystem::path& relativePath);

private:
    void searchAssets();
    std::filesystem::path
    toAbsolutePath(const std::filesystem::path& relativePath) const; // Relative to assets root folder

    bool importMesh(const std::filesystem::path& relativePath) const;
    bool importShader(const std::filesystem::path& relativePath) const;
    bool importTexture(const std::filesystem::path& relativePath) const;
    bool importMaterial(const std::filesystem::path& relativePath) const;
    bool importModel(const std::filesystem::path& relativePath) const;

    AssetStorage& storage_;
    std::unordered_set<std::filesystem::path> availableAssetFiles_;
    std::filesystem::path root_;
};
