#pragma once
#include <memory>
#include <string>
#include "core/AssetManager.hpp"
#include "core/assets/types/Mesh.hpp"

class PrimitiveBuilder {
public:
    explicit PrimitiveBuilder(AssetManager& assetManager) : assetManager_(assetManager) {}

    std::string createCube() const {
        auto mesh = MeshFactory::createPositionMesh(PRIMITIVE_GEOMETRY_CUBE, getCubeVertices(), getCubeIndices());
        auto meshPtr = std::make_unique<Mesh>(std::move(mesh));
        assetManager_.insertMesh(std::move(meshPtr));
        return PRIMITIVE_GEOMETRY_CUBE;
    }

    std::string createTriangle() const {
        auto mesh = MeshFactory::createPositionMesh(PRIMITIVE_GEOMETRY_TRIANGLE, getTriangleVertices());
        auto meshPtr = std::make_unique<Mesh>(std::move(mesh));
        assetManager_.insertMesh(std::move(meshPtr));
        return PRIMITIVE_GEOMETRY_TRIANGLE;
    }

private:
    static const std::vector<PositionVertex>& getCubeVertices() {
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
        // clang-format on
        return CUBE_VERTICES;
    }

    static const std::vector<uint32_t>& getCubeIndices() {
        // clang-format off
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
        return CUBE_INDICES;
    }

    static const std::vector<PositionVertex>& getTriangleVertices() {
        // clang-format off
        static const std::vector<PositionVertex> TRIANGLE_VERTICES = {
            {{ 0.0f, -0.5f, 0.0f}}, // Bottom
            {{-0.5f,  0.5f, 0.0f}}, // Top-left
            {{ 0.5f,  0.5f, 0.0f}}  // Top-right
        };
        // clang-format on
        return TRIANGLE_VERTICES;
    }

    static constexpr auto PRIMITIVE_GEOMETRY_CUBE = "_primitive_geometry_cube";
    static constexpr auto PRIMITIVE_GEOMETRY_TRIANGLE = "_primitive_geometry_triangle";

    AssetManager& assetManager_;
};
