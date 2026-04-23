#pragma once
#include <filesystem>
#include <glm/vec4.hpp>
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

struct MeshDescriptor {
    std::string name;
    std::string type;
    std::string meshFile;
    bool ccw;

    static MeshDescriptor fromFile(const std::filesystem::path& path) {
        auto j = JsonUtils::loadJson(path);
        return {.name = JsonUtils::getRequired<std::string>(j, "name"),
                .type = JsonUtils::getRequired<std::string>(j, "type"),
                .meshFile = JsonUtils::getRequired<std::string>(j, "meshFile"),
                .ccw = JsonUtils::getOptional<bool>(j, "ccw", true)};
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

struct MaterialDescriptor {
    std::string name;
    std::string type;
    std::string shaderName;
    glm::vec4 baseColor;
    std::string textureName;

    static MaterialDescriptor fromFile(const std::filesystem::path& path) {
        auto j = JsonUtils::loadJson(path);
        return {.name = JsonUtils::getRequired<std::string>(j, "name"),
                .type = JsonUtils::getRequired<std::string>(j, "type"),
                .shaderName = JsonUtils::getOptional<std::string>(j, "shaderName", "unlit_textured"),
                .baseColor = JsonUtils::getOptional<glm::vec4>(j, "baseColor", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}),
                .textureName = JsonUtils::getOptional<std::string>(j, "textureName", "checkers")};
    }
};
