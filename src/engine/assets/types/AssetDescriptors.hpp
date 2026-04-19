#pragma once
#include <filesystem>
#include <string>
#include "utils/JsonUtils.hpp"

struct AssetDescriptor {
    std::string name;
    std::string type;

    static AssetDescriptor fromFile(const std::filesystem::path& path) {
        auto j = JsonUtils::loadJson(path);
        return {.name = JsonUtils::getRequired<std::string>(j, "name"),
                .type = JsonUtils::getRequired<std::string>(j, "type")};
    }
};

struct MeshDataDescriptor {
    std::string name;
    std::string type;
    std::string meshFile;

    static MeshDataDescriptor fromFile(const std::filesystem::path& path) {
        auto j = JsonUtils::loadJson(path);
        return {.name = JsonUtils::getRequired<std::string>(j, "name"),
                .type = JsonUtils::getRequired<std::string>(j, "type"),
                .meshFile = JsonUtils::getRequired<std::string>(j, "meshFile")};
    }
};

struct ShaderDescriptor {
    std::string name;
    std::string type;
    std::string vertexShader;
    std::string fragmentShader;

    static ShaderDescriptor fromFile(const std::filesystem::path& path) {
        auto j = JsonUtils::loadJson(path);
        return {.name = JsonUtils::getRequired<std::string>(j, "name"),
                .type = JsonUtils::getRequired<std::string>(j, "type"),
                .vertexShader = JsonUtils::getRequired<std::string>(j, "vertexShader"),
                .fragmentShader = JsonUtils::getRequired<std::string>(j, "fragmentShader")};
    }
};

struct TextureDescriptor {
    std::string name;
    std::string type;
    std::string image;

    static TextureDescriptor fromFile(const std::filesystem::path& path) {
        auto j = JsonUtils::loadJson(path);
        return {.name = JsonUtils::getRequired<std::string>(j, "name"),
                .type = JsonUtils::getRequired<std::string>(j, "type"),
                .image = JsonUtils::getRequired<std::string>(j, "image")};
    }
};
