#pragma once
#include <array>
#include <cstring>
#include <glm/glm.hpp>
#include <vector>
#include "../../../../asset_management/asset_types/Mesh.hpp"

struct VertexAttribute {
    uint32_t offset;
    uint32_t componentCount;
};

struct VertexLayout_PositionOnly {
    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t VERTEX_STRIDE = 3;
    static constexpr std::array<VertexAttribute, 1> VERTEX_ATTRIBS = {{{POSITION_OFFSET, POSITION_COMPONENTS}}};
};

struct VertexLayout_PositionColor {
    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t COLOR_OFFSET = 3;
    static constexpr uint32_t COLOR_COMPONENTS = 3;
    static constexpr uint32_t VERTEX_STRIDE = 6;
    static constexpr std::array<VertexAttribute, 2> VERTEX_ATTRIBS = {
            {{POSITION_OFFSET, POSITION_COMPONENTS}, {COLOR_OFFSET, COLOR_COMPONENTS}}};
};

struct VertexLayout_PositionUv {
    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t UV_OFFSET = 3;
    static constexpr uint32_t UV_COMPONENTS = 2;
    static constexpr uint32_t VERTEX_STRIDE = 5;
    static constexpr std::array<VertexAttribute, 2> VERTEX_ATTRIBS = {
            {{POSITION_OFFSET, POSITION_COMPONENTS}, {UV_OFFSET, UV_COMPONENTS}}};
};

struct VertexLayout_PositionNormalUv {
    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t NORMAL_OFFSET = 3;
    static constexpr uint32_t NORMAL_COMPONENTS = 3;
    static constexpr uint32_t UV_OFFSET = 6;
    static constexpr uint32_t UV_COMPONENTS = 2;
    static constexpr uint32_t VERTEX_STRIDE = 8;
    static constexpr std::array<VertexAttribute, 3> VERTEX_ATTRIBS = {
            {{POSITION_OFFSET, POSITION_COMPONENTS}, {NORMAL_OFFSET, NORMAL_COMPONENTS}, {UV_OFFSET, UV_COMPONENTS}}};
};

// pos(3) + normal(3) + uv(2) + boneIndices(4 as float) + boneWeights(4) = 16 floats
struct VertexLayout_PositionNormalUvSkinned {
    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t NORMAL_OFFSET = 3;
    static constexpr uint32_t NORMAL_COMPONENTS = 3;
    static constexpr uint32_t UV_OFFSET = 6;
    static constexpr uint32_t UV_COMPONENTS = 2;
    static constexpr uint32_t BONE_IDX_OFFSET = 8;
    static constexpr uint32_t BONE_IDX_COMPONENTS = 4; // packed as ivec4 → VK_FORMAT_R32G32B32A32_SINT
    static constexpr uint32_t BONE_WT_OFFSET = 12;
    static constexpr uint32_t BONE_WT_COMPONENTS = 4;
    static constexpr uint32_t VERTEX_STRIDE = 16;
};

inline std::vector<float> packMeshData(const Mesh& mesh) {
    std::vector<float> data;
    const auto& positions = mesh.getPositions();
    const auto& normals = mesh.getNormals();
    const auto& uvs = mesh.getUvs();

    data.reserve(positions.size() * VertexLayout_PositionNormalUv::VERTEX_STRIDE);
    for (size_t i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        const glm::vec3 normal = i < normals.size() ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
        data.push_back(normal.x);
        data.push_back(normal.y);
        data.push_back(normal.z);
        const glm::vec2 uv = i < uvs.size() ? uvs[i] : glm::vec2(0.0f);
        data.push_back(uv.x);
        data.push_back(uv.y);
    }
    return data;
}

// Packs position + normal + uv + boneIndices(as float bits) + boneWeights.
// boneIndices are stored as reinterpreted int bits so we keep a single float[] VBO.
inline std::vector<float> packSkinnedMeshData(const Mesh& mesh) {
    std::vector<float> data;
    const auto& positions = mesh.getPositions();
    const auto& normals = mesh.getNormals();
    const auto& uvs = mesh.getUvs();
    const auto& boneIndices = mesh.getBoneIndices();
    const auto& boneWeights = mesh.getBoneWeights();

    data.reserve(positions.size() * VertexLayout_PositionNormalUvSkinned::VERTEX_STRIDE);
    for (size_t i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);

        const glm::vec3 normal = i < normals.size() ? normals[i] : glm::vec3(0.0f, 1.0f, 0.0f);
        data.push_back(normal.x);
        data.push_back(normal.y);
        data.push_back(normal.z);

        const glm::vec2 uv = i < uvs.size() ? uvs[i] : glm::vec2(0.0f);
        data.push_back(uv.x);
        data.push_back(uv.y);

        const glm::ivec4 bi = i < boneIndices.size() ? boneIndices[i] : glm::ivec4(0);
        // Reinterpret int bits as float so we can store in the same float VBO.
        float bx, by, bz, bw;
        std::memcpy(&bx, &bi.x, sizeof(float));
        std::memcpy(&by, &bi.y, sizeof(float));
        std::memcpy(&bz, &bi.z, sizeof(float));
        std::memcpy(&bw, &bi.w, sizeof(float));
        data.push_back(bx);
        data.push_back(by);
        data.push_back(bz);
        data.push_back(bw);

        const glm::vec4 bw4 = i < boneWeights.size() ? boneWeights[i] : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        data.push_back(bw4.x);
        data.push_back(bw4.y);
        data.push_back(bw4.z);
        data.push_back(bw4.w);
    }
    return data;
}
