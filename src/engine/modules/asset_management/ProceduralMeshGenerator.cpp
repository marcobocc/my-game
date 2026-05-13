#include "ProceduralMeshGenerator.hpp"

std::unique_ptr<MeshResource> ProceduralMeshGenerator::dispatch(const std::string& name) {
    if (name.find("SPHERE") != std::string::npos) {
        size_t lastUnderscore = name.rfind('_');
        if (lastUnderscore != std::string::npos) {
            uint32_t resolution = std::stoul(name.substr(lastUnderscore + 1));
            return generateSphere(resolution, name);
        }
    } else if (name.find("CUBE") != std::string::npos) {
        return generateCube(name);
    } else if (name.find("PLANE") != std::string::npos) {
        return generatePlane(name);
    }
    throw std::runtime_error("Unknown procedural mesh type: " + name);
}

std::unique_ptr<MeshResource> ProceduralMeshGenerator::generateSphere(uint32_t resolution, const std::string& name) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
    const float radius = 1.0f;
    for (uint32_t lat = 0; lat <= resolution; ++lat) {
        float theta = glm::pi<float>() * static_cast<float>(lat) / static_cast<float>(resolution);
        float sinTheta = glm::sin(theta);
        float cosTheta = glm::cos(theta);
        for (uint32_t lon = 0; lon <= resolution; ++lon) {
            float phi = 2.0f * glm::pi<float>() * static_cast<float>(lon) / static_cast<float>(resolution);
            float sinPhi = glm::sin(phi);
            float cosPhi = glm::cos(phi);
            // Position on unit sphere
            glm::vec3 normal(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
            glm::vec3 position = normal * radius;
            positions.push_back(position);
            normals.push_back(normal);
            // UV coordinates
            float u = static_cast<float>(lon) / static_cast<float>(resolution);
            float v = static_cast<float>(lat) / static_cast<float>(resolution);
            uvs.push_back({u, v});
        }
    }
    // Generate indices
    for (uint32_t lat = 0; lat < resolution; ++lat) {
        for (uint32_t lon = 0; lon < resolution; ++lon) {
            uint32_t a = lat * (resolution + 1) + lon;
            uint32_t b = a + resolution + 1;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);
            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }

    std::vector<glm::vec3> colors(positions.size(), glm::vec3(1.0f, 1.0f, 1.0f));
    return std::make_unique<MeshResource>(name, positions, uvs, colors, indices, normals);
}

std::unique_ptr<MeshResource> ProceduralMeshGenerator::generateCube(const std::string& name) {
    std::vector<glm::vec3> positions = {// Front (+Z)
                                        {-0.5f, -0.5f, 0.5f},
                                        {0.5f, -0.5f, 0.5f},
                                        {0.5f, 0.5f, 0.5f},
                                        {-0.5f, 0.5f, 0.5f},
                                        // Back (-Z)
                                        {0.5f, -0.5f, -0.5f},
                                        {-0.5f, -0.5f, -0.5f},
                                        {-0.5f, 0.5f, -0.5f},
                                        {0.5f, 0.5f, -0.5f},
                                        // Left (-X)
                                        {-0.5f, -0.5f, -0.5f},
                                        {-0.5f, -0.5f, 0.5f},
                                        {-0.5f, 0.5f, 0.5f},
                                        {-0.5f, 0.5f, -0.5f},
                                        // Right (+X)
                                        {0.5f, -0.5f, 0.5f},
                                        {0.5f, -0.5f, -0.5f},
                                        {0.5f, 0.5f, -0.5f},
                                        {0.5f, 0.5f, 0.5f},
                                        // Top (+Y)
                                        {-0.5f, 0.5f, 0.5f},
                                        {0.5f, 0.5f, 0.5f},
                                        {0.5f, 0.5f, -0.5f},
                                        {-0.5f, 0.5f, -0.5f},
                                        // Bottom (-Y)
                                        {-0.5f, -0.5f, -0.5f},
                                        {0.5f, -0.5f, -0.5f},
                                        {0.5f, -0.5f, 0.5f},
                                        {-0.5f, -0.5f, 0.5f}};

    std::vector<glm::vec2> uvs(24, {0, 0});
    for (int i = 0; i < 6; ++i) {
        uvs[i * 4 + 0] = {0, 0};
        uvs[i * 4 + 1] = {1, 0};
        uvs[i * 4 + 2] = {1, 1};
        uvs[i * 4 + 3] = {0, 1};
    }
    std::vector<glm::vec3> colors(24, glm::vec3(1.0f, 1.0f, 1.0f));
    std::vector<glm::vec3> normals = {// Front
                                      {0, 0, 1},
                                      {0, 0, 1},
                                      {0, 0, 1},
                                      {0, 0, 1},
                                      // Back
                                      {0, 0, -1},
                                      {0, 0, -1},
                                      {0, 0, -1},
                                      {0, 0, -1},
                                      // Left
                                      {-1, 0, 0},
                                      {-1, 0, 0},
                                      {-1, 0, 0},
                                      {-1, 0, 0},
                                      // Right
                                      {1, 0, 0},
                                      {1, 0, 0},
                                      {1, 0, 0},
                                      {1, 0, 0},
                                      // Top
                                      {0, 1, 0},
                                      {0, 1, 0},
                                      {0, 1, 0},
                                      {0, 1, 0},
                                      // Bottom
                                      {0, -1, 0},
                                      {0, -1, 0},
                                      {0, -1, 0},
                                      {0, -1, 0}};

    std::vector<uint32_t> indices = {0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
                                     12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22};

    return std::make_unique<MeshResource>(name, positions, uvs, colors, indices, normals);
}

std::unique_ptr<MeshResource> ProceduralMeshGenerator::generatePlane(const std::string& name) {
    std::vector<glm::vec3> positions = {
            {-0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, 0.5f}, {-0.5f, 0.0f, 0.5f}};
    std::vector<glm::vec2> uvs = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    std::vector<glm::vec3> colors(4, glm::vec3(1.0f, 1.0f, 1.0f));
    std::vector<glm::vec3> normals = {{0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}};
    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
    return std::make_unique<MeshResource>(name, positions, uvs, colors, indices, normals);
}
