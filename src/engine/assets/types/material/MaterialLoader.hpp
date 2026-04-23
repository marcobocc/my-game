#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <string>
#include "assets/AssetCache.hpp"
#include "assets/types/AssetDescriptors.hpp"
#include "assets/types/base/AssetLoader.hpp"
#include "assets/types/material/Material.hpp"

class MaterialLoader : public AssetLoader {
    static constexpr std::string ASSET_TYPE_STR = "material";
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("MaterialLoader");

public:
    bool load(const std::filesystem::path& assetDescriptorFile, AssetCache& dest) override {
        MaterialDescriptor def = MaterialDescriptor::fromFile(assetDescriptorFile);
        auto material = std::make_unique<Material>(def.name, def.shaderName, def.baseColor, def.textureName);
        dest.insert<Material>(def.name, std::move(material));
        return true;
    }

    std::string getAssetType() override { return ASSET_TYPE_STR; }
};
