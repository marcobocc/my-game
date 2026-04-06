#pragma once
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include "core/assets/loaders/MaterialLoader.hpp"
#include "core/assets/loaders/MeshLoader.hpp"
#include "core/assets/loaders/ShaderLoader.hpp"
#include "core/assets/types/Material.hpp"
#include "core/assets/types/Mesh.hpp"
#include "core/assets/types/ShaderPipeline.hpp"

class AssetManager {
public:
    // ---------------- SHADER ----------------
    ShaderPipeline* getShader(const std::string& name) { return getAsset(name, shaderCache_, shaderLoader_); }
    void insertShader(std::unique_ptr<ShaderPipeline> shader) { shaderCache_[shader->name] = std::move(shader); }

    // ---------------- MESH ----------------
    Mesh* getMesh(const std::string& name) { return getAsset(name, meshCache_, meshLoader_); }
    void insertMesh(std::unique_ptr<Mesh> mesh) { meshCache_[mesh->name] = std::move(mesh); }

    // ---------------- MATERIAL ----------------
    Material* getMaterial(const std::string& name) {
        Material* mat = getAsset(name, materialCache_, materialLoader_);
        getShader(mat->shaderPipelineName);
        return mat;
    }
    void insertMaterial(std::unique_ptr<Material> material) { materialCache_[material->name] = std::move(material); }

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

    std::unordered_map<std::string, std::unique_ptr<Mesh>> meshCache_;
    std::unordered_map<std::string, std::unique_ptr<ShaderPipeline>> shaderCache_;
    std::unordered_map<std::string, std::unique_ptr<Material>> materialCache_;

    MeshLoader meshLoader_;
    ShaderLoader shaderLoader_;
    MaterialLoader materialLoader_;
};
