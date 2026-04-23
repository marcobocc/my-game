#pragma once
#include <filesystem>
#include <glm/vec4.hpp>
#include <string>
#include "utils/JsonUtils.hpp"

struct MeshDescriptor {
    std::string name;
    std::string meshFile;
    bool ccw;

    static MeshDescriptor fromFile(const std::filesystem::path& path, const std::string& name) {
        auto j = JsonUtils::loadJson(path);
        return {.name = name,
                .meshFile = JsonUtils::getRequired<std::string>(j, "meshFile"),
                .ccw = JsonUtils::getOptional<bool>(j, "ccw", true)};
    }
};

struct ShaderDescriptor {
    std::string name;
    std::string vertexShader;
    std::string fragmentShader;

    static ShaderDescriptor fromFile(const std::filesystem::path& path, const std::string& name) {
        auto j = JsonUtils::loadJson(path);
        return {.name = name,
                .vertexShader = JsonUtils::getRequired<std::string>(j, "vertexShader"),
                .fragmentShader = JsonUtils::getRequired<std::string>(j, "fragmentShader")};
    }
};

struct TextureDescriptor {
    std::string name;
    std::string image;

    static TextureDescriptor fromFile(const std::filesystem::path& path, const std::string& name) {
        auto j = JsonUtils::loadJson(path);
        return {.name = name, .image = JsonUtils::getRequired<std::string>(j, "image")};
    }
};

struct MaterialDescriptor {
    std::string name;
    std::string shaderName;
    glm::vec4 baseColor;
    std::string textureName;

    static MaterialDescriptor fromFile(const std::filesystem::path& path, const std::string& name) {
        auto j = JsonUtils::loadJson(path);
        return {.name = name,
                .shaderName = JsonUtils::getOptional<std::string>(j, "shaderName", "unlit_textured.shad"),
                .baseColor = JsonUtils::getOptional<glm::vec4>(j, "baseColor", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}),
                .textureName = JsonUtils::getOptional<std::string>(j, "textureName", "white1x1.tex")};
    }
};
