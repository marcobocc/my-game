#pragma once
#include <string>
#include <vector>
#include "assets/types/base/Asset.hpp"
#include "details/VertexLayouts.hpp"

class Mesh final : public Asset {
public:
    Mesh(const std::string& name,
         const std::vector<Vertex_WithLayout_PositionOnly>& vertices,
         const std::vector<uint32_t>& indices) :
        Asset(name) {
        std::vector<float> data;
        for (const auto& [position]: vertices) {
            data.push_back(position.x);
            data.push_back(position.y);
            data.push_back(position.z);
        }
        vertices_ = std::move(data);
        indices_ = indices;
        vertexAttributes_ = std::vector(Vertex_WithLayout_PositionOnly::VERTEX_ATTRIBS.begin(),
                                        Vertex_WithLayout_PositionOnly::VERTEX_ATTRIBS.end());
        vertexStride_ = Vertex_WithLayout_PositionOnly::VERTEX_STRIDE;
    }

    Mesh(const std::string& name,
         const std::vector<Vertex_WithLayout_PositionUv>& vertices,
         const std::vector<uint32_t>& indices) :
        Asset(name) {
        std::vector<float> data;
        for (const auto& [position, uv]: vertices) {
            data.push_back(position.x);
            data.push_back(position.y);
            data.push_back(position.z);
            data.push_back(uv.x);
            data.push_back(uv.y);
        }
        vertices_ = std::move(data);
        indices_ = indices;
        vertexAttributes_ = std::vector(Vertex_WithLayout_PositionUv::VERTEX_ATTRIBS.begin(),
                                        Vertex_WithLayout_PositionUv::VERTEX_ATTRIBS.end());
        vertexStride_ = Vertex_WithLayout_PositionUv::VERTEX_STRIDE;
    }

    Mesh(const std::string& name,
         const std::vector<Vertex_WithLayout_PositionColor>& vertices,
         const std::vector<uint32_t>& indices) :
        Asset(name) {
        std::vector<float> data;
        for (const auto& [position, color]: vertices) {
            data.push_back(position.x);
            data.push_back(position.y);
            data.push_back(position.z);
            data.push_back(color.r);
            data.push_back(color.g);
            data.push_back(color.b);
        }
        vertices_ = std::move(data);
        indices_ = indices;
        vertexAttributes_ = std::vector(Vertex_WithLayout_PositionColor::VERTEX_ATTRIBS.begin(),
                                        Vertex_WithLayout_PositionColor::VERTEX_ATTRIBS.end());
        vertexStride_ = Vertex_WithLayout_PositionColor::VERTEX_STRIDE;
    }

    size_t getVertexCount() const { return vertices_.size() / vertexStride_; }
    bool hasIndices() const { return !indices_.empty(); }
    const std::vector<float>& getVertices() const { return vertices_; }
    const std::vector<uint32_t>& getIndices() const { return indices_; }
    const std::vector<VertexAttribute>& getVertexAttributes() const { return vertexAttributes_; }
    uint32_t getVertexStride() const { return vertexStride_; }

private:
    std::vector<float> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<VertexAttribute> vertexAttributes_;
    uint32_t vertexStride_ = 0;
};
