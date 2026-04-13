#pragma once
#include <memory>

class Texture {
public:
    ~Texture() = default;
    Texture(int width, int height, int channels, std::vector<unsigned char> data)
        : width_(width), height_(height), channels_(channels), data_(std::move(data)) {}

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    int getChannels() const { return channels_; }
    const unsigned char* getData() const { return data_.data(); }

private:
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
    std::vector<unsigned char> data_;
};
