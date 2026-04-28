#pragma once
#include <array>
#include <glm/glm.hpp>
#include <vector>
#include "assets/types/Mesh.hpp"

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

struct VertexLayout_PositionUv {
    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t UV_OFFSET = 3;
    static constexpr uint32_t UV_COMPONENTS = 2;
    static constexpr uint32_t VERTEX_STRIDE = 5;
    static constexpr std::array<VertexAttribute, 2> VERTEX_ATTRIBS = {
            {{POSITION_OFFSET, POSITION_COMPONENTS}, {UV_OFFSET, UV_COMPONENTS}}};
};

inline std::vector<float> packMeshData(const Mesh& mesh) {
    std::vector<float> data;
    const auto& positions = mesh.getPositions();
    const auto& uvs = mesh.getUvs();

    data.reserve(positions.size() * VertexLayout_PositionUv::VERTEX_STRIDE);
    for (size_t i = 0; i < positions.size(); ++i) {
        data.push_back(positions[i].x);
        data.push_back(positions[i].y);
        data.push_back(positions[i].z);
        const glm::vec2 uv = (i < uvs.size()) ? uvs[i] : glm::vec2(0.0f);
        data.push_back(uv.x);
        data.push_back(uv.y);
    }
    return data;
}
