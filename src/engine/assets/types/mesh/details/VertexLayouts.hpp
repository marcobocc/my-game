#pragma once
#include <glm/glm.hpp>

struct VertexAttribute {
    uint32_t offset;
    uint32_t componentCount;
};

struct Vertex_WithLayout_PositionOnly {
    glm::vec3 position;

    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t VERTEX_STRIDE = 3;
    static constexpr std::array<VertexAttribute, 1> VERTEX_ATTRIBS = {{{POSITION_OFFSET, POSITION_COMPONENTS}}};
};

struct Vertex_WithLayout_PositionColor {
    glm::vec3 position;
    glm::vec3 color;

    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t COLOR_OFFSET = 3;
    static constexpr uint32_t COLOR_COMPONENTS = 3;

    static constexpr uint32_t VERTEX_STRIDE = 6;
    static constexpr std::array<VertexAttribute, 2> VERTEX_ATTRIBS = {
            {{POSITION_OFFSET, POSITION_COMPONENTS}, {COLOR_OFFSET, COLOR_COMPONENTS}}};
};

struct Vertex_WithLayout_PositionUv {
    glm::vec3 position;
    glm::vec2 uv;

    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t UV_OFFSET = 3;
    static constexpr uint32_t UV_COMPONENTS = 2;
    static constexpr uint32_t VERTEX_STRIDE = 5;
    static constexpr std::array<VertexAttribute, 2> VERTEX_ATTRIBS = {
            {{POSITION_OFFSET, POSITION_COMPONENTS}, {UV_OFFSET, UV_COMPONENTS}}};
};
