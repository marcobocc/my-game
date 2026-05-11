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
    bool tangentVertexLayout;

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
                .positionColorVertexLayout = JsonUtils::getOptional<bool>(j, "positionColorVertexLayout", false),
                .tangentVertexLayout = JsonUtils::getOptional<bool>(j, "tangentVertexLayout", false)};
    }
};

struct MaterialDescriptor {
    std::string name;
    std::string shaderName;
    glm::vec4 tint;
    std::string albedoTexture;
    std::string normalTexture;
    std::string roughnessTexture;
    std::string metallicTexture;
    std::string aoTexture;
    float metallic;
    float roughness;
    float ao;

    static MaterialDescriptor fromFile(const std::filesystem::path& absolutePath, const std::string& name) {
        auto j = JsonUtils::loadJson(absolutePath);
        return {.name = name,
                .shaderName = JsonUtils::getOptional<std::string>(j, "shaderName", GBUFFER_SHADER),
                .tint = JsonUtils::getOptional<glm::vec4>(j, "tint", glm::vec4{1.0f, 1.0f, 1.0f, 1.0f}),
                .albedoTexture = JsonUtils::getOptional<std::string>(j, "albedoTexture", EMPTY_TEXTURE),
                .normalTexture = JsonUtils::getOptional<std::string>(j, "normalTexture", EMPTY_TEXTURE),
                .roughnessTexture = JsonUtils::getOptional<std::string>(j, "roughnessTexture", EMPTY_TEXTURE),
                .metallicTexture = JsonUtils::getOptional<std::string>(j, "metallicTexture", EMPTY_TEXTURE),
                .aoTexture = JsonUtils::getOptional<std::string>(j, "aoTexture", EMPTY_TEXTURE),
                .metallic = JsonUtils::getOptional<float>(j, "metallic", 0.0f),
                .roughness = JsonUtils::getOptional<float>(j, "roughness", 1.0f),
                .ao = JsonUtils::getOptional<float>(j, "ao", 1.0f)};
    }
};

struct ModelDescriptor {
    std::string name;
    std::string meshName;
    std::string materialName;

    static ModelDescriptor fromFile(const std::filesystem::path& absolutePath, const std::string& name) {
        auto j = JsonUtils::loadJson(absolutePath);
        return {.name = name,
                .meshName = JsonUtils::getRequired<std::string>(j, "meshName"),
                .materialName = JsonUtils::getRequired<std::string>(j, "materialName")};
    }
};
