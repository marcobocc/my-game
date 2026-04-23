#pragma once
#include <filesystem>
#include <string>

class AssetCache;

class AssetLoader {
public:
    virtual ~AssetLoader() = default;
    virtual bool load(const std::filesystem::path& assetDescriptorFile, const std::string& name, AssetCache& dest) = 0;
};
