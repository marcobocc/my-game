#pragma once
#include <string>
#include <vector>
#include "Asset.hpp"

class Texture final : public Asset {
public:
    Texture(const std::string& name, int width, int height, int channels, std::vector<unsigned char> imageData) :
        Asset(name),
        width_(width),
        height_(height),
        channels_(channels),
        imageData_(std::move(imageData)) {}

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getChannels() const { return channels_; }
    const unsigned char* getImageData() const { return imageData_.data(); }

private:
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    std::vector<unsigned char> imageData_;
};
