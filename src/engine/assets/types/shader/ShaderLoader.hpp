#pragma once
#include <filesystem>
#include <fstream>
#include <log4cxx/logger.h>
#include <string>
#include "Shader.hpp"
#include "assets/AssetCache.hpp"
#include "assets/types/AssetDescriptors.hpp"
#include "assets/types/base/AssetLoader.hpp"

class ShaderLoader : public AssetLoader {
    static constexpr std::string ASSET_TYPE_STR = "shader";
    inline static const log4cxx::LoggerPtr LOGGER = log4cxx::Logger::getLogger("ShaderLoader");

public:
    bool load(const std::filesystem::path& assetDescriptorFile, AssetCache& dest) override {
        ShaderDescriptor def = ShaderDescriptor::fromFile(assetDescriptorFile);
        auto assetFolder = assetDescriptorFile.parent_path();
        auto vertexFilePath = assetFolder / def.vertexShader;
        auto fragmentFilePath = assetFolder / def.fragmentShader;
        std::vector<char> vertexBytecode, fragmentBytecode;
        try {
            vertexBytecode = readShaderFile(vertexFilePath);
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, e.what());
            return false;
        }
        try {
            fragmentBytecode = readShaderFile(fragmentFilePath);
        } catch (const std::exception& e) {
            LOG4CXX_ERROR(LOGGER, e.what());
            return false;
        }
        auto shader = std::make_unique<Shader>(def.name, std::move(vertexBytecode), std::move(fragmentBytecode));
        dest.insert<Shader>(def.name, std::move(shader));
        return true;
    }

    std::string getAssetType() override { return ASSET_TYPE_STR; }

private:
    static std::vector<char> readShaderFile(const std::filesystem::path& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open shader file: " + filePath.string());
        }
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("Failed to read shader file: " + filePath.string());
        }
        return buffer;
    }
};
