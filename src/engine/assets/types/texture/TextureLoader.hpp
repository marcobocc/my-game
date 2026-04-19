#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <string>
#include "assets/AssetCache.hpp"
#include "assets/types/AssetDescriptors.hpp"
#include "assets/types/base/AssetLoader.hpp"
#include "assets/types/texture/Texture.hpp"
#include "stb_image.h"

class TextureLoader : public AssetLoader {
    static constexpr std::string ASSET_TYPE_STR = "texture";
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("TextureLoader");

public:
    bool load(const std::filesystem::path& assetDescriptorFile, AssetCache& dest) override {
        TextureDescriptor def = TextureDescriptor::fromFile(assetDescriptorFile);
        auto assetFolder = assetDescriptorFile.parent_path();
        auto imageFilePath = assetFolder / def.image;
        int w, h, c;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* data = stbi_load(imageFilePath.c_str(), &w, &h, &c, 4);
        if (!data) {
            LOG4CXX_ERROR(LOGGER, "Failed to load image file: " << imageFilePath);
            return false;
        }
        size_t size = w * h * 4;
        std::vector<unsigned char> buffer(data, data + size);
        stbi_image_free(data);
        auto texture = std::make_unique<Texture>(def.name, w, h, 4, std::move(buffer));
        dest.insert<Texture>(def.name, std::move(texture));
        return true;
    }

    std::string getAssetType() override { return ASSET_TYPE_STR; }
};
