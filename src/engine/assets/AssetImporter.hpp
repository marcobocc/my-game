#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <string>
#include <unordered_map>

class AssetStorage;

class AssetImporter {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetImporter");

public:
    explicit AssetImporter(const std::filesystem::path& root, AssetStorage& cache);
    bool import(const std::string& name);

private:
    void searchAssets(const std::filesystem::path& root);

    bool importMesh(const std::filesystem::path& file, const std::string& name) const;
    bool importShader(const std::filesystem::path& file, const std::string& name) const;
    bool importTexture(const std::filesystem::path& file, const std::string& name) const;
    bool importMaterial(const std::filesystem::path& file, const std::string& name) const;

    AssetStorage& storage_;
    std::unordered_map<std::string, std::filesystem::path> availableAssetFiles_;
};
