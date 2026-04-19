#pragma once

class AssetCache;

class AssetLoader {
public:
    virtual ~AssetLoader() = default;
    virtual bool load(const std::filesystem::path& assetDescriptorFile, AssetCache& dest) = 0;
    virtual std::string getAssetType() = 0;
};
