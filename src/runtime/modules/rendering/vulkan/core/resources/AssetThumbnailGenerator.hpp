#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include "../../../../asset_management/asset_types/Texture.hpp"
#include "ImguiThumbnail.hpp"
#include "VulkanTextureCache.hpp"
#include "backends/imgui_impl_vulkan.h"
#include "modules/asset_management/AssetLoader.hpp"

class AssetThumbnailGenerator {
public:
    static constexpr const char* PREVIEW_NOT_AVAILABLE_PATH = "assets/editor/preview_not_available.jpg";

    AssetThumbnailGenerator(AssetLoader& assetLoader, VulkanTextureCache& textureCache) :
        assetLoader_(assetLoader),
        textureCache_(textureCache) {}

    ImguiThumbnail get(const std::string& assetName) {
        auto it = cache_.find(assetName);
        if (it != cache_.end()) return it->second;

        std::string ext = std::filesystem::path(assetName).extension().string();
        std::optional<ImguiThumbnail> result;
        if (ext == ".png" || ext == ".jpg") {
            result = getTexturePreview(assetName);
        } else if (ext == ".mesh") {
            result = getMeshPreview(assetName);
        } else if (ext == ".mat") {
            result = getMaterialPreview(assetName);
        } else if (ext == ".shad") {
            result = getShaderPreview(assetName);
        } else if (ext == ".model") {
            result = getModelPreview(assetName);
        }

        ImguiThumbnail thumbnail = result.value_or(thumbnail_PreviewNotAvailable());
        cache_.emplace(assetName, thumbnail);
        return thumbnail;
    }

private:
    AssetLoader& assetLoader_;
    VulkanTextureCache& textureCache_;
    std::unordered_map<std::string, ImguiThumbnail> cache_;

    ImguiThumbnail toImguiThumbnail(const Texture& texture) {
        VulkanTexture& vulkanTexture = textureCache_.get(texture);
        VkDescriptorSet id = ImGui_ImplVulkan_AddTexture(
                vulkanTexture.sampler, vulkanTexture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        return ImguiThumbnail{id, vulkanTexture.width, vulkanTexture.height};
    }

    ImguiThumbnail thumbnail_PreviewNotAvailable() {
        const Texture* texture = assetLoader_.get<Texture>(PREVIEW_NOT_AVAILABLE_PATH);
        if (!texture) return ImguiThumbnail{};
        return toImguiThumbnail(*texture);
    }

    std::optional<ImguiThumbnail> getTexturePreview(const std::string& assetName) {
        const Texture* texture = assetLoader_.get<Texture>(assetName);
        if (!texture) return std::nullopt;
        return toImguiThumbnail(*texture);
    }

    std::optional<ImguiThumbnail> getMeshPreview(const std::string&) { return std::nullopt; }

    std::optional<ImguiThumbnail> getMaterialPreview(const std::string&) { return std::nullopt; }

    std::optional<ImguiThumbnail> getShaderPreview(const std::string&) { return std::nullopt; }

    std::optional<ImguiThumbnail> getModelPreview(const std::string&) { return std::nullopt; }
};
