#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include "core/assets/loaders/MeshLoader.hpp"
#include "core/assets/loaders/ShaderLoader.hpp"
#include "core/assets/loaders/TextureLoader.hpp"
#include "core/assets/types/MeshData.hpp"
#include "core/assets/types/ShaderPipeline.hpp"
#include "core/assets/types/Texture.hpp"

class AssetManager {
public:
    explicit AssetManager(const std::filesystem::path& assetsPath) :
        meshLoader_(assetsPath),
        shaderLoader_(assetsPath),
        textureLoader_(assetsPath),
        assetsPath_(assetsPath) {}

    // ---------------- SHADER ----------------
    ShaderPipeline* getShader(const std::string& name) { return getAsset(name, shaderCache_, shaderLoader_); }
    void insertShader(std::unique_ptr<ShaderPipeline> shader) { shaderCache_[shader->name] = std::move(shader); }

    // ---------------- MESH ----------------
    MeshData* getMesh(const std::string& name) { return getAsset(name, meshCache_, meshLoader_); }
    void insertMesh(std::unique_ptr<MeshData> mesh) { meshCache_[mesh->name] = std::move(mesh); }

    // ---------------- TEXTURE ----------------
    Texture* getTexture(const std::string& path) { return getAsset(path, textureCache_, textureLoader_); }
    void insertTexture(std::unique_ptr<Texture> tex, const std::string& key) { textureCache_[key] = std::move(tex); }

private:
    template<typename AssetType, typename LoaderType>
    AssetType* getAsset(const std::string& name,
                        std::unordered_map<std::string, std::unique_ptr<AssetType>>& cache,
                        const LoaderType& loader) {
        auto it = cache.find(name);
        if (it != cache.end()) return it->second.get();

        std::unique_ptr<AssetType> asset;
        asset = loader.load(name);
        AssetType* ptr = asset.get();
        cache[name] = std::move(asset);
        return ptr;
    }

    std::unordered_map<std::string, std::unique_ptr<MeshData>> meshCache_;
    std::unordered_map<std::string, std::unique_ptr<ShaderPipeline>> shaderCache_;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textureCache_;

    MeshLoader meshLoader_;
    ShaderLoader shaderLoader_;
    TextureLoader textureLoader_;
    std::filesystem::path assetsPath_;
};
