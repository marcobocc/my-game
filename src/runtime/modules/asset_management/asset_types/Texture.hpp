#pragma once
#include <string>
#include <vector>
#include "Asset.hpp"

class Texture final : public Asset {
public:
    Texture(const std::string& name, int width, int height, int channels, std::vector<unsigned char> imageData) :
        Asset(name),
        width(width),
        height(height),
        channels(channels),
        imageData(std::move(imageData)) {}

    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> imageData;
};
