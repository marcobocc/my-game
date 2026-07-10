#pragma once
#include <filesystem>
#include <log4cxx/logger.h>
#include <memory>
#include <nlohmann/json.hpp>
#include "AssetCache.hpp"
#include "MeshImporter.hpp"
#include "VirtualFileSystem.hpp"
#include "animation/assets/AnimationClip.hpp"
#include "animation/assets/AnimatorController.hpp"
#include "animation/assets/Skeleton.hpp"
#include "graphics/assets/Font.hpp"
#include "graphics/assets/Material.hpp"
#include "graphics/assets/Mesh.hpp"
#include "graphics/assets/Model.hpp"
#include "graphics/assets/Shader.hpp"
#include "graphics/assets/Texture.hpp"
#include "scripting/assets/LuaScript.hpp"
#include "stb_image.h"
#include "stb_truetype.h"
#include "utils/JsonUtils.hpp"

class LuaScriptSystem;

class AssetLoader {
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("AssetLoader");

public:
    explicit AssetLoader(VirtualFileSystem& vfs, AssetCache& cache) : vfs_(vfs), cache_(cache) {}

    template<typename T>
    T* get(const std::string& name) {
        if (!cache_.contains(name)) load<T>(name);
        if (!cache_.contains(name)) return nullptr;
        return cache_.get<T>(name);
    }

    void scanAndRegisterScripts(LuaScriptSystem& luaScriptSystem);

private:
    template<typename T>
    std::unique_ptr<T> load(const std::string& name) const;

    VirtualFileSystem& vfs_;
    AssetCache& cache_;
};

template<>
inline std::unique_ptr<Mesh> AssetLoader::load<Mesh>(const std::string& name) const {
    if (!vfs_.exists(name)) {
        LOG4CXX_ERROR(LOGGER, "Asset not found: " << name);
        return nullptr;
    }
    std::string meshFilePath = name;
    bool reverseWinding = false;
    if (std::filesystem::path(name).extension() == ".mesh") {
        try {
            auto meshData = vfs_.read(name);
            auto meshStr = std::string(meshData.begin(), meshData.end());
            auto j = nlohmann::json::parse(meshStr);
            meshFilePath = JsonUtils::getRequired<std::string>(j, "meshFile");
            bool ccw = JsonUtils::getOptional<bool>(j, "ccw", true);
            reverseWinding = !ccw;
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, "Failed to read mesh metadata: " << name << " - " << e.what());
            return nullptr;
        }
    }
    try {
        auto imported = importing::importMeshFile(meshFilePath, reverseWinding, name, vfs_);
        if (imported.meshes.empty()) return nullptr;

        // Collect clip names before moving clips into the cache.
        std::vector<std::string> clipNames;
        clipNames.reserve(imported.clips.size());
        for (const auto& clip: imported.clips)
            clipNames.push_back(clip->name);

        // Insert companion skeleton and clips into the cache.
        if (imported.skeleton) {
            LOG4CXX_INFO(LOGGER, "Loaded skeleton: " << imported.skeleton->name);
            cache_.insert<Skeleton>(std::move(imported.skeleton));
        }
        for (auto& clip: imported.clips) {
            LOG4CXX_INFO(LOGGER, "Loaded animation clip: " << clip->name);
            cache_.insert<AnimationClip>(std::move(clip));
        }

        auto& mesh = imported.meshes[0];
        mesh->name = name; // ensure cache key matches the .mesh metadata filename
        mesh->setClipNames(clipNames);
        auto meshPtr = mesh.get();
        cache_.insert<Mesh>(std::move(mesh));
        LOG4CXX_INFO(LOGGER, "Successfully loaded mesh: " << name);
        return std::make_unique<Mesh>(*meshPtr);
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to import mesh: " << name << " - " << e.what());
        return nullptr;
    }
}

template<>
inline std::unique_ptr<Skeleton> AssetLoader::load<Skeleton>(const std::string& name) const {
    // Skeletons are always inserted as companions during Mesh loading; they are never loaded standalone.
    LOG4CXX_WARN(LOGGER, "Standalone Skeleton load requested for '" << name << "' — load the mesh first.");
    return nullptr;
}

template<>
inline std::unique_ptr<AnimationClip> AssetLoader::load<AnimationClip>(const std::string& name) const {
    // AnimationClips are always inserted as companions during Mesh loading; they are never loaded standalone.
    LOG4CXX_WARN(LOGGER, "Standalone AnimationClip load requested for '" << name << "' — load the mesh first.");
    return nullptr;
}

template<>
inline std::unique_ptr<Shader> AssetLoader::load<Shader>(const std::string& name) const {
    try {
        auto shaderDataVec = vfs_.read(name);
        auto shaderStr = std::string(shaderDataVec.begin(), shaderDataVec.end());
        auto j = nlohmann::json::parse(shaderStr);

        bool disableCull = JsonUtils::getOptional<bool>(j, "disableCull", false);
        bool disableDepthTest = JsonUtils::getOptional<bool>(j, "disableDepthTest", false);
        bool disableDepthWrite = JsonUtils::getOptional<bool>(j, "disableDepthWrite", false);
        bool enableAlphaBlend = JsonUtils::getOptional<bool>(j, "enableAlphaBlend", false);
        bool noVertexInput = JsonUtils::getOptional<bool>(j, "noVertexInput", false);
        bool lineTopology = JsonUtils::getOptional<bool>(j, "lineTopology", false);
        bool positionColorVertexLayout = JsonUtils::getOptional<bool>(j, "positionColorVertexLayout", false);
        bool tangentVertexLayout = JsonUtils::getOptional<bool>(j, "tangentVertexLayout", false);
        bool depthBias = JsonUtils::getOptional<bool>(j, "depthBias", false);
        bool depthLessOrEqual = JsonUtils::getOptional<bool>(j, "depthLessOrEqual", false);
        bool skinnedVertexLayout = JsonUtils::getOptional<bool>(j, "skinnedVertexLayout", false);
        bool textVertexLayout = JsonUtils::getOptional<bool>(j, "textVertexLayout", false);

        auto readBytecode = [&](const std::string& path) -> std::vector<char> {
            auto data = vfs_.read(path);
            return std::vector<char>(data.begin(), data.end());
        };

        std::vector<char> vertexBytecode;
        std::vector<char> fragmentBytecode;
        std::vector<char> computeBytecode;

        if (j.contains("computeShader")) {
            computeBytecode = readBytecode(JsonUtils::getRequired<std::string>(j, "computeShader"));
        } else {
            vertexBytecode = readBytecode(JsonUtils::getRequired<std::string>(j, "vertexShader"));
            fragmentBytecode = readBytecode(JsonUtils::getRequired<std::string>(j, "fragmentShader"));
        }

        auto shader = std::make_unique<Shader>(name,
                                               std::move(vertexBytecode),
                                               std::move(fragmentBytecode),
                                               disableCull,
                                               disableDepthTest,
                                               disableDepthWrite,
                                               enableAlphaBlend,
                                               noVertexInput,
                                               lineTopology,
                                               positionColorVertexLayout,
                                               tangentVertexLayout,
                                               depthBias,
                                               depthLessOrEqual,
                                               std::move(computeBytecode),
                                               skinnedVertexLayout,
                                               textVertexLayout);
        if (shader) {
            auto shaderPtr = shader.get();
            cache_.insert<Shader>(std::move(shader));
            LOG4CXX_INFO(LOGGER, "Successfully loaded shader: " << name);
            return std::make_unique<Shader>(*shaderPtr);
        }
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load shader: " << name << " - " << e.what());
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Texture> AssetLoader::load<Texture>(const std::string& name) const {
    if (!vfs_.exists(name)) {
        LOG4CXX_ERROR(LOGGER, "Texture file not found in VFS: " << name);
        return nullptr;
    }
    auto imageData = vfs_.read(name);
    if (imageData.empty()) {
        LOG4CXX_ERROR(LOGGER, "Failed to read texture data: " << name);
        return nullptr;
    }
    int width, height, channels;
    unsigned char* data =
            stbi_load_from_memory(imageData.data(), imageData.size(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        LOG4CXX_ERROR(LOGGER, "Failed to decode texture: " << name << " - " << stbi_failure_reason());
        return nullptr;
    }
    constexpr int forcedChannels = 4;
    std::vector<unsigned char> decodedData(data, data + width * height * forcedChannels);
    stbi_image_free(data);
    auto texture = std::make_unique<Texture>(name, width, height, forcedChannels, std::move(decodedData));
    if (texture) {
        auto texturePtr = texture.get();
        cache_.insert<Texture>(std::move(texture));
        LOG4CXX_INFO(LOGGER, "Successfully loaded texture: " << name);
        return std::make_unique<Texture>(*texturePtr);
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Material> AssetLoader::load<Material>(const std::string& name) const {
    try {
        auto materialDataVec = vfs_.read(name);
        auto materialStr = std::string(materialDataVec.begin(), materialDataVec.end());
        auto j = nlohmann::json::parse(materialStr);
        auto material = std::make_unique<Material>(Material::deserialize(j, name));
        if (material) {
            auto materialPtr = material.get();
            cache_.insert<Material>(std::move(material));
            LOG4CXX_INFO(LOGGER, "Successfully loaded material: " << name);
            return std::make_unique<Material>(*materialPtr);
        }
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load material: " << name << " - " << e.what());
    }
    return nullptr;
}

template<>
inline std::unique_ptr<AnimatorController> AssetLoader::load<AnimatorController>(const std::string& name) const {
    try {
        auto dataVec = vfs_.read(name);
        auto str = std::string(dataVec.begin(), dataVec.end());
        auto j = nlohmann::json::parse(str);
        auto asset = std::make_unique<AnimatorController>(AnimatorController::deserialize(j, name));
        auto* ptr = asset.get();
        cache_.insert<AnimatorController>(std::move(asset));
        LOG4CXX_INFO(LOGGER, "Successfully loaded animator controller: " << name);
        return std::make_unique<AnimatorController>(*ptr);
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load animator controller: " << name << " - " << e.what());
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Model> AssetLoader::load<Model>(const std::string& name) const {
    try {
        auto modelDataVec = vfs_.read(name);
        auto modelStr = std::string(modelDataVec.begin(), modelDataVec.end());
        auto j = nlohmann::json::parse(modelStr);
        Model def = Model::deserialize(j, name);
        auto model = std::make_unique<Model>(std::move(def));
        if (model) {
            auto modelPtr = model.get();
            cache_.insert<Model>(std::move(model));
            LOG4CXX_INFO(LOGGER, "Successfully loaded model: " << name);
            return std::make_unique<Model>(*modelPtr);
        }
    } catch (const std::exception& e) {
        LOG4CXX_ERROR(LOGGER, "Failed to load model: " << name << " - " << e.what());
    }
    return nullptr;
}

template<>
inline std::unique_ptr<Font> AssetLoader::load<Font>(const std::string& name) const {
    if (!vfs_.exists(name)) {
        LOG4CXX_ERROR(LOGGER, "Font file not found in VFS: " << name);
        return nullptr;
    }
    auto ttfData = vfs_.read(name);
    if (ttfData.empty()) {
        LOG4CXX_ERROR(LOGGER, "Failed to read font data: " << name);
        return nullptr;
    }

    constexpr float FONT_SIZE = 32.0f;
    constexpr int ATLAS_W = 512;
    constexpr int ATLAS_H = 512;
    constexpr int FIRST_CHAR = 32; // space
    constexpr int NUM_CHARS = 96; // printable ASCII

    std::vector<stbtt_bakedchar> charData(NUM_CHARS);
    std::vector<uint8_t> alphaAtlas(ATLAS_W * ATLAS_H, 0);

    int bakeResult = stbtt_BakeFontBitmap(
            ttfData.data(), 0, FONT_SIZE, alphaAtlas.data(), ATLAS_W, ATLAS_H, FIRST_CHAR, NUM_CHARS, charData.data());

    if (bakeResult == 0) {
        LOG4CXX_ERROR(LOGGER, "Font atlas too small for: " << name);
        return nullptr;
    }

    // Expand single-channel alpha atlas to RGBA8 (R=G=B=1, A=coverage).
    std::vector<uint8_t> rgbaAtlas(ATLAS_W * ATLAS_H * 4);
    for (int i = 0; i < ATLAS_W * ATLAS_H; ++i) {
        rgbaAtlas[i * 4 + 0] = 255;
        rgbaAtlas[i * 4 + 1] = 255;
        rgbaAtlas[i * 4 + 2] = 255;
        rgbaAtlas[i * 4 + 3] = alphaAtlas[i];
    }

    // Extract per-glyph metrics.
    std::unordered_map<uint32_t, Font::Glyph> glyphs;
    glyphs.reserve(NUM_CHARS);
    for (int i = 0; i < NUM_CHARS; ++i) {
        const auto& bc = charData[i];
        uint32_t codepoint = static_cast<uint32_t>(FIRST_CHAR + i);
        Font::Glyph g{};
        g.u0 = bc.x0 / static_cast<float>(ATLAS_W);
        g.v0 = bc.y0 / static_cast<float>(ATLAS_H);
        g.u1 = bc.x1 / static_cast<float>(ATLAS_W);
        g.v1 = bc.y1 / static_cast<float>(ATLAS_H);
        g.bearingX = bc.xoff;
        g.bearingY = bc.yoff;
        g.advance = bc.xadvance;
        g.width = bc.x1 - bc.x0;
        g.height = bc.y1 - bc.y0;
        glyphs.emplace(codepoint, g);
    }

    // Extract global metrics via stbtt_fontinfo for ascent/descent/lineGap.
    stbtt_fontinfo info{};
    stbtt_InitFont(&info, ttfData.data(), stbtt_GetFontOffsetForIndex(ttfData.data(), 0));
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);
    float scale = stbtt_ScaleForPixelHeight(&info, FONT_SIZE);

    auto font = std::make_unique<Font>(name,
                                       static_cast<uint32_t>(ATLAS_W),
                                       static_cast<uint32_t>(ATLAS_H),
                                       std::move(rgbaAtlas),
                                       std::move(glyphs),
                                       ascent * scale,
                                       descent * scale,
                                       lineGap * scale,
                                       FONT_SIZE);

    auto* ptr = font.get();
    cache_.insert<Font>(std::move(font));
    LOG4CXX_INFO(LOGGER, "Successfully loaded font: " << name);
    return std::make_unique<Font>(*ptr);
}

template<>
inline std::unique_ptr<LuaScript> AssetLoader::load<LuaScript>(const std::string& name) const {
    auto realPath = vfs_.getRealPath(name);
    if (!realPath) {
        LOG4CXX_ERROR(LOGGER, "Lua script not found in VFS: " << name);
        return nullptr;
    }
    auto script = std::make_unique<LuaScript>(name, *realPath);
    auto* ptr = script.get();
    cache_.insert<LuaScript>(std::move(script));
    LOG4CXX_INFO(LOGGER, "Successfully loaded lua script: " << name);
    return std::make_unique<LuaScript>(*ptr);
}
