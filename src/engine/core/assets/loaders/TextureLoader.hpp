#pragma once
#include <string>
#include <memory>
#include "core/assets/types/Texture.hpp"
#include "stb_image.h"
#include <stdexcept>

class TextureLoader {
public:
    explicit TextureLoader(const std::filesystem::path& assetsPath) : assetsPath_(assetsPath) {}

    std::unique_ptr<Texture> load(const std::string& textureName) const {
        auto path = assetsPath_ / "textures" / textureName;
        int width = 0, height = 0, channels = 0;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* data = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
        if (!data) {
            throw std::runtime_error("Failed to load texture: " + path.string());
        }
        size_t dataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
        std::vector<unsigned char> vecData(data, data + dataSize);
        stbi_image_free(data);
        return std::make_unique<Texture>(width, height, 4, std::move(vecData));
    }

private:
    std::filesystem::path assetsPath_;
};
