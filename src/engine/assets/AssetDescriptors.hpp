#pragma once
#include <filesystem>
#include <glm/vec4.hpp>
#include <string>
#include "BuiltinAssetNames.hpp"
#include "utils/JsonUtils.hpp"

struct MeshDescriptor {
    std::string name;
    std::string meshFile;
    bool ccw;

    static MeshDescriptor fromFile(const std::filesystem::path& absolutePath, const std::string& name) {
        auto j = JsonUtils::loadJson(absolutePath);
        return {.name = name,
                .meshFile = JsonUtils::getRequired<std::string>(j, "meshFile"),
                .ccw = JsonUtils::getOptional<bool>(j, "ccw", true)};
    }
};

struct ShaderDescriptor {
    std::string name;
    std::string vertexShader;
    std::string fragmentShader;
    bool disableCull;
    bool disableDepthTest;
    bool disableDepthWrite;
    bool enableAlphaBlend;
    bool noVertexInput;
    bool lineTopology;
    bool positionColorVertexLayout;

    static ShaderDescriptor fromFile(const std::filesystem::path& absolutePath, const std::string& name) {
        auto j = JsonUtils::loadJson(absolutePath);
        return {.name = name,
                .vertexShader = JsonUtils::getRequired<std::string>(j, "vertexShader"),
                .fragmentShader = JsonUtils::getRequired<std::string>(j, "fragmentShader"),
                .disableCull = JsonUtils::getOptional<bool>(j, "disableCull", false),
                .disableDepthTest = JsonUtils::getOptional<bool>(j, "disableDepthTest", false),
                .disableDepthWrite = JsonUtils::getOptional<bool>(j, "disableDepthWrite", false),
                .enableAlphaBlend = JsonUtils::getOptional<bool>(j, "enableAlphaBlend", false),
                .noVertexInput = JsonUtils::getOptional<bool>(j, "noVertexInput", false),
                .lineTopology = JsonUtils::getOptional<bool>(j, "lineTopology", false),
                .positionColorVertexLayout = JsonUtils::getOptional<bool>(j, "positionColorVertexLayout", false)};
    }
};

struct MaterialDescriptor {
    std::string name;
    std::string shaderName;
    glm::vec4 baseColor;
    std::string textureName;

    static MaterialDescriptor fromFile(const std::filesystem::path& absolutePath, const std::string& name) {
        auto j = JsonUtils::loadJson(absolutePath);
        return {.name = name,
                .shaderName = JsonUtils::getOptional<std::string>(j, "shaderName", GBUFFER_SHADER),
                .baseColor = JsonUtils::getOptional<glm::vec4>(j, "baseColor", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}),
                .textureName = JsonUtils::getOptional<std::string>(j, "textureName", EMPTY_TEXTURE)};
    }
};
