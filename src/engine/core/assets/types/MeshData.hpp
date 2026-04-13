#pragma once
#include <array>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct VertexAttribute {
    uint32_t offset;
    uint32_t componentCount;
};

// Position-only vertex for material-colored geometry
struct PositionVertex {
    glm::vec3 position;

    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t VERTEX_STRIDE = 3;
    static constexpr std::array<VertexAttribute, 1> VERTEX_ATTRIBS = {{{POSITION_OFFSET, POSITION_COMPONENTS}}};
};

// Position + Color vertex for vertex-colored geometry
struct PositionColorVertex {
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

// Position + UV vertex for textured geometry
struct PositionUVVertex {
    glm::vec3 position;
    glm::vec2 uv;

    static constexpr uint32_t POSITION_OFFSET = 0;
    static constexpr uint32_t POSITION_COMPONENTS = 3;
    static constexpr uint32_t UV_OFFSET = 3;
    static constexpr uint32_t UV_COMPONENTS = 2;
    static constexpr uint32_t VERTEX_STRIDE = 5;
    static constexpr std::array<VertexAttribute, 2> VERTEX_ATTRIBS = {
        {{POSITION_OFFSET, POSITION_COMPONENTS}, {UV_OFFSET, UV_COMPONENTS}}
    };
};

struct MeshData {
    std::string name{};
    std::vector<float> vertices{};
    std::vector<uint32_t> indices{};
    std::vector<VertexAttribute> vertexAttributes{};
    uint32_t vertexStride{};

    size_t getVertexCount() const { return vertices.size() / vertexStride; }
    bool hasIndices() const { return !indices.empty(); }
};

class MeshFactory {
public:
    // Create mesh with position-only vertices (colors come from material)
    static MeshData createPositionMesh(const std::string& name,
                                       const std::vector<PositionVertex>& vertices,
                                       const std::vector<uint32_t>& indices = {}) {
        std::vector<float> data;
        for (const auto& vertex: vertices) {
            data.push_back(vertex.position.x);
            data.push_back(vertex.position.y);
            data.push_back(vertex.position.z);
        }

        return {.name = name,
                .vertices = data,
                .indices = indices,
                .vertexAttributes =
                        std::vector(PositionVertex::VERTEX_ATTRIBS.begin(), PositionVertex::VERTEX_ATTRIBS.end()),
                .vertexStride = PositionVertex::VERTEX_STRIDE};
    }

    // Create mesh with position + color vertices
    static MeshData createPositionColorMesh(const std::string& name,
                                            const std::vector<PositionColorVertex>& vertices,
                                            const std::vector<uint32_t>& indices = {}) {
        std::vector<float> data;
        for (const auto& [position, color]: vertices) {
            data.push_back(position.x);
            data.push_back(position.y);
            data.push_back(position.z);
            data.push_back(color.r);
            data.push_back(color.g);
            data.push_back(color.b);
        }

        return {.name = name,
                .vertices = data,
                .indices = indices,
                .vertexAttributes = std::vector(PositionColorVertex::VERTEX_ATTRIBS.begin(),
                                                PositionColorVertex::VERTEX_ATTRIBS.end()),
                .vertexStride = PositionColorVertex::VERTEX_STRIDE};
    }

    // Create mesh with position + UV vertices
    static MeshData createPositionUVMesh(const std::string& name,
                                         const std::vector<PositionUVVertex>& vertices,
                                         const std::vector<uint32_t>& indices = {}) {
        std::vector<float> data;
        for (const auto& vertex: vertices) {
            data.push_back(vertex.position.x);
            data.push_back(vertex.position.y);
            data.push_back(vertex.position.z);
            data.push_back(vertex.uv.x);
            data.push_back(vertex.uv.y);
        }
        return {.name = name,
                .vertices = data,
                .indices = indices,
                .vertexAttributes = std::vector(PositionUVVertex::VERTEX_ATTRIBS.begin(),
                                                PositionUVVertex::VERTEX_ATTRIBS.end()),
                .vertexStride = PositionUVVertex::VERTEX_STRIDE};
    }
};
