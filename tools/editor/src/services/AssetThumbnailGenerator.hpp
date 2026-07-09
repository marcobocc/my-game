#pragma once
#include <filesystem>
#include <fstream>
#include <log4cxx/logger.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "ImguiThumbnail.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "modules/asset_management/AssetLoader.hpp"
#include "modules/asset_management/asset_types/Texture.hpp"
#include "modules/rendering/vulkan/core/resources/VulkanTextureCache.hpp"
#include "stb_image.h"
#include "stb_image_write.h"

class AssetThumbnailGenerator {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetThumbnailGenerator");

    static constexpr auto PREVIEW_NOT_AVAILABLE_PATH = "assets/internal.editor/images/preview_not_available.jpg";
    static constexpr auto CACHE_DIR = ".cache/thumbnails";
    static constexpr int THUMBNAIL_SIZE = 128;
    static constexpr int JPEG_QUALITY = 90;

public:
    AssetThumbnailGenerator(AssetLoader& assetLoader,
                            VulkanTextureCache& textureCache,
                            std::filesystem::path projectRoot) :
        assetLoader_(assetLoader),
        textureCache_(textureCache),
        cacheDir_(std::move(projectRoot) / CACHE_DIR) {}

    void setProjectRoot(const std::filesystem::path& projectRoot) { cacheDir_ = projectRoot / CACHE_DIR; }

    ImguiThumbnail get(const std::string& assetName) {
        if (assetName.starts_with(CACHE_DIR)) return ImguiThumbnail{};
        auto it = cache_.find(assetName);
        if (it != cache_.end()) return it->second;

        auto ext = std::filesystem::path(assetName).extension().string();
        std::optional<ImguiThumbnail> result;
        if (ext == ".png" || ext == ".jpg") {
            auto osCache = osCachePathFor(assetName);
            if (std::filesystem::exists(osCache)) result = loadThumbnailFromDisk(osCache);
            if (!result) result = generateTexturePreview(assetName);
        }

        ImguiThumbnail thumbnail = result.value_or(thumbnail_PreviewNotAvailable());
        cache_.emplace(assetName, thumbnail);
        return thumbnail;
    }

private:
    AssetLoader& assetLoader_;
    VulkanTextureCache& textureCache_;
    std::filesystem::path cacheDir_;
    std::unordered_map<std::string, ImguiThumbnail> cache_;

    std::filesystem::path osCachePathFor(const std::string& vfsPath) const {
        std::string encoded = vfsPath;
        for (char& c: encoded)
            if (c == '/') c = '_';
        return cacheDir_ / (encoded + ".jpg");
    }

    void ensureCacheDir() { std::filesystem::create_directories(cacheDir_); }

    std::optional<ImguiThumbnail> loadThumbnailFromDisk(const std::filesystem::path& path) {
        int w = 0, h = 0, channels = 0;
        unsigned char* data = stbi_load(path.string().c_str(), &w, &h, &channels, STBI_rgb_alpha);
        if (!data) return std::nullopt;
        std::vector<unsigned char> pixels(data, data + w * h * 4);
        stbi_image_free(data);
        Texture texture(path.string(), w, h, 4, std::move(pixels));
        return toImguiThumbnail(texture);
    }

    ImguiThumbnail toImguiThumbnail(const Texture& texture) {
        VulkanTexture& vulkanTexture = textureCache_.get(texture);
        VkDescriptorSet id = ImGui_ImplVulkan_AddTexture(
                vulkanTexture.sampler, vulkanTexture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return ImguiThumbnail{id, vulkanTexture.width, vulkanTexture.height};
    }

    ImguiThumbnail thumbnail_PreviewNotAvailable() {
        auto it = cache_.find(PREVIEW_NOT_AVAILABLE_PATH);
        if (it != cache_.end()) return it->second;
        const Texture* texture = assetLoader_.get<Texture>(PREVIEW_NOT_AVAILABLE_PATH);
        ImguiThumbnail thumbnail = texture ? toImguiThumbnail(*texture) : ImguiThumbnail{};
        cache_.emplace(PREVIEW_NOT_AVAILABLE_PATH, thumbnail);
        return thumbnail;
    }

    static std::vector<unsigned char> downsample(const std::vector<unsigned char>& src, int srcW, int srcH) {
        std::vector<unsigned char> dst(THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4);
        float scaleX = static_cast<float>(srcW) / THUMBNAIL_SIZE;
        float scaleY = static_cast<float>(srcH) / THUMBNAIL_SIZE;
        for (int y = 0; y < THUMBNAIL_SIZE; ++y) {
            for (int x = 0; x < THUMBNAIL_SIZE; ++x) {
                float srcX = (x + 0.5f) * scaleX - 0.5f;
                float srcY = (y + 0.5f) * scaleY - 0.5f;
                int x0 = std::max(0, static_cast<int>(srcX));
                int y0 = std::max(0, static_cast<int>(srcY));
                int x1 = std::min(srcW - 1, x0 + 1);
                int y1 = std::min(srcH - 1, y0 + 1);
                float tx = srcX - static_cast<float>(x0);
                float ty = srcY - static_cast<float>(y0);
                for (int c = 0; c < 4; ++c) {
                    float p00 = static_cast<float>(src[(y0 * srcW + x0) * 4 + c]);
                    float p10 = static_cast<float>(src[(y0 * srcW + x1) * 4 + c]);
                    float p01 = static_cast<float>(src[(y1 * srcW + x0) * 4 + c]);
                    float p11 = static_cast<float>(src[(y1 * srcW + x1) * 4 + c]);
                    float val = p00 * (1 - tx) * (1 - ty) + p10 * tx * (1 - ty) + p01 * (1 - tx) * ty + p11 * tx * ty;
                    dst[(y * THUMBNAIL_SIZE + x) * 4 + c] = static_cast<unsigned char>(val);
                }
            }
        }
        return dst;
    }

    void writeToDiskCache(const std::string& assetName, const Texture& texture) {
        ensureCacheDir();
        auto thumb = downsample(texture.imageData, texture.width, texture.height);
        std::vector<unsigned char> jpegBytes;
        stbi_write_jpg_to_func(
                [](void* ctx, void* data, int size) {
                    auto* vec = static_cast<std::vector<unsigned char>*>(ctx);
                    auto* bytes = static_cast<unsigned char*>(data);
                    vec->insert(vec->end(), bytes, bytes + size);
                },
                &jpegBytes,
                THUMBNAIL_SIZE,
                THUMBNAIL_SIZE,
                4,
                thumb.data(),
                JPEG_QUALITY);
        if (jpegBytes.empty()) return;
        auto osPath = osCachePathFor(assetName);
        std::ofstream out(osPath, std::ios::binary);
        out.write(reinterpret_cast<const char*>(jpegBytes.data()), static_cast<std::streamsize>(jpegBytes.size()));
    }

    std::optional<ImguiThumbnail> generateTexturePreview(const std::string& assetName) {
        const Texture* texture = assetLoader_.get<Texture>(assetName);
        if (!texture) return std::nullopt;
        writeToDiskCache(assetName, *texture);
        return toImguiThumbnail(*texture);
    }
};
