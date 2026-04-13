#include "core/assets/loaders/prefabs/PrefabMesh.hpp"
#include <vector>
#include "core/assets/PrefabNames.hpp"
#include "core/assets/types/MeshData.hpp"

std::unique_ptr<MeshData> PrefabMesh::makeCube() {
    // clang-format off
    static const std::vector<PositionVertex> CUBE_VERTICES = {
        // Front face
        {{-0.5f, -0.5f,  0.5f}}, // 0
        {{ 0.5f, -0.5f,  0.5f}}, // 1
        {{ 0.5f,  0.5f,  0.5f}}, // 2
        {{-0.5f,  0.5f,  0.5f}}, // 3
        // Back face
        {{-0.5f, -0.5f, -0.5f}}, // 4
        {{ 0.5f, -0.5f, -0.5f}}, // 5
        {{ 0.5f,  0.5f, -0.5f}}, // 6
        {{-0.5f,  0.5f, -0.5f}}  // 7
    };
    static const std::vector<uint32_t> CUBE_INDICES = {
        // Front
        0, 2, 1, 0, 3, 2,
        // Back
        4, 5, 6, 4, 6, 7,
        // Left
        4, 7, 3, 4, 3, 0,
        // Right
        1, 2, 6, 1, 6, 5,
        // Top
        3, 7, 6, 3, 6, 2,
        // Bottom
        4, 0, 1, 4, 1, 5
    };
    // clang-format on
    auto mesh = MeshFactory::createPositionMesh(PRIMITIVE_GEOMETRY_CUBE, CUBE_VERTICES, CUBE_INDICES);
    return std::make_unique<MeshData>(std::move(mesh));
}

std::unique_ptr<MeshData> PrefabMesh::makeTriangle() {
    // clang-format off
    static const std::vector<PositionVertex> TRIANGLE_VERTICES = {
        {{ 0.0f, -0.5f, 0.0f}}, // Bottom
        {{-0.5f,  0.5f, 0.0f}}, // Top-left
        {{ 0.5f,  0.5f, 0.0f}}  // Top-right
    };
    // clang-format on
    auto mesh = MeshFactory::createPositionMesh(PRIMITIVE_GEOMETRY_TRIANGLE, TRIANGLE_VERTICES);
    return std::make_unique<MeshData>(std::move(mesh));
}
