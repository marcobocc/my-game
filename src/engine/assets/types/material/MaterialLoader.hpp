#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <string>
#include "assets/AssetCache.hpp"
#include "assets/types/AssetDescriptors.hpp"
#include "assets/types/base/AssetLoader.hpp"
#include "assets/types/material/Material.hpp"

class MaterialLoader : public AssetLoader {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("MaterialLoader");

public:
    bool load(const std::filesystem::path& assetDescriptorFile, const std::string& name, AssetCache& dest) override {
        MaterialDescriptor def = MaterialDescriptor::fromFile(assetDescriptorFile, name);
        auto material = std::make_unique<Material>(name, def.shaderName, def.baseColor, def.textureName);
        dest.insert<Material>(name, std::move(material));
        return true;
    }
};
