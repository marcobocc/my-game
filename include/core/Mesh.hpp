#pragma once
#include <string>
#include <vector>

struct VertexAttribute {
    std::string name; // "position", "color", etc.
    uint32_t offset; // offset in floats from start of vertex
    uint32_t componentCount; // 2 for vec2, 3 for vec3, 4 for vec4
};

struct Mesh {
    std::string name;
    std::vector<float> vertices; // flattened vertex data
    std::vector<uint32_t> indices; // optional index buffer
    uint32_t vertexStride = 0; // floats per vertex
    std::vector<VertexAttribute> attributes; // logical attributes

    size_t getVertexCount() const { return vertices.size() / vertexStride; }
    bool hasIndices() const { return !indices.empty(); }
};
