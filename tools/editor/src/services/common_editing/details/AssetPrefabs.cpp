#include "AssetPrefabs.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

Mesh AssetPrefabs::sphere(uint32_t resolution, const std::string& name) {
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
            glm::vec3 normal(sinTheta * cosPhi, cosTheta, sinTheta * sinPhi);
            glm::vec3 position = normal * radius;
            positions.push_back(position);
            normals.push_back(normal);
            float u = static_cast<float>(lon) / static_cast<float>(resolution);
            float v = static_cast<float>(lat) / static_cast<float>(resolution);
            uvs.push_back({u, v});
        }
    }
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
    std::vector<glm::vec3> colors(positions.size(), glm::vec3(1.0f));
    return Mesh(name, positions, uvs, colors, indices, normals);
}

Mesh AssetPrefabs::cube(const std::string& name) {
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
    std::vector<glm::vec3> colors(24, glm::vec3(1.0f));
    std::vector<glm::vec3> normals = {{0, 0, 1},  {0, 0, 1},  {0, 0, 1},  {0, 0, 1},  {0, 0, -1}, {0, 0, -1},
                                      {0, 0, -1}, {0, 0, -1}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0},
                                      {1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {1, 0, 0},  {0, 1, 0},  {0, 1, 0},
                                      {0, 1, 0},  {0, 1, 0},  {0, -1, 0}, {0, -1, 0}, {0, -1, 0}, {0, -1, 0}};
    std::vector<uint32_t> indices = {0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
                                     12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22};
    return Mesh(name, positions, uvs, colors, indices, normals);
}

Mesh AssetPrefabs::plane(const std::string& name) {
    std::vector<glm::vec3> positions = {
            {-0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, -0.5f}, {0.5f, 0.0f, 0.5f}, {-0.5f, 0.0f, 0.5f}};
    std::vector<glm::vec2> uvs = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    std::vector<glm::vec3> colors(4, glm::vec3(1.0f));
    std::vector<glm::vec3> normals = {{0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}};
    std::vector<uint32_t> indices = {0, 1, 2, 0, 2, 3};
    return Mesh(name, positions, uvs, colors, indices, normals);
}

Mesh AssetPrefabs::capsule(uint32_t resolution, const std::string& name) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    const float radius = 0.5f;
    const float halfHeight = 0.5f; // cylinder half-height (caps sit above/below this)
    const uint32_t rings = resolution / 2; // rings per hemisphere

    auto addVertex = [&](glm::vec3 pos, glm::vec3 normal, glm::vec2 uv) {
        positions.push_back(pos);
        normals.push_back(normal);
        uvs.push_back(uv);
    };

    // Top hemisphere (lat goes from 0 to pi/2)
    for (uint32_t lat = 0; lat <= rings; ++lat) {
        float theta = glm::half_pi<float>() * static_cast<float>(lat) / static_cast<float>(rings);
        float sinTheta = glm::sin(theta);
        float cosTheta = glm::cos(theta);
        for (uint32_t lon = 0; lon <= resolution; ++lon) {
            float phi = 2.0f * glm::pi<float>() * static_cast<float>(lon) / static_cast<float>(resolution);
            glm::vec3 n(sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi));
            glm::vec3 p = n * radius + glm::vec3(0.0f, halfHeight, 0.0f);
            float u = static_cast<float>(lon) / static_cast<float>(resolution);
            float v = 0.5f + 0.5f * (1.0f - static_cast<float>(lat) / static_cast<float>(rings));
            addVertex(p, n, {u, v});
        }
    }

    // Bottom hemisphere (lat goes from pi/2 to pi)
    for (uint32_t lat = 0; lat <= rings; ++lat) {
        float theta =
                glm::half_pi<float>() + glm::half_pi<float>() * static_cast<float>(lat) / static_cast<float>(rings);
        float sinTheta = glm::sin(theta);
        float cosTheta = glm::cos(theta);
        for (uint32_t lon = 0; lon <= resolution; ++lon) {
            float phi = 2.0f * glm::pi<float>() * static_cast<float>(lon) / static_cast<float>(resolution);
            glm::vec3 n(sinTheta * glm::cos(phi), cosTheta, sinTheta * glm::sin(phi));
            glm::vec3 p = n * radius + glm::vec3(0.0f, -halfHeight, 0.0f);
            float u = static_cast<float>(lon) / static_cast<float>(resolution);
            float v = 0.5f * (1.0f - static_cast<float>(lat) / static_cast<float>(rings));
            addVertex(p, n, {u, v});
        }
    }

    uint32_t stride = resolution + 1;
    uint32_t topOffset = 0;
    uint32_t botOffset = (rings + 1) * stride;

    for (uint32_t lat = 0; lat < rings; ++lat) {
        for (uint32_t lon = 0; lon < resolution; ++lon) {
            uint32_t a = topOffset + lat * stride + lon;
            uint32_t b = a + stride;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);
            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }
    // Cylinder body: stitch top equator (last top ring) to bottom equator (first bottom ring)
    for (uint32_t lon = 0; lon < resolution; ++lon) {
        uint32_t a = topOffset + rings * stride + lon;
        uint32_t b = botOffset + lon;
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(a + 1);
        indices.push_back(a + 1);
        indices.push_back(b);
        indices.push_back(b + 1);
    }
    for (uint32_t lat = 0; lat < rings; ++lat) {
        for (uint32_t lon = 0; lon < resolution; ++lon) {
            uint32_t a = botOffset + lat * stride + lon;
            uint32_t b = a + stride;
            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);
            indices.push_back(a + 1);
            indices.push_back(b);
            indices.push_back(b + 1);
        }
    }

    std::vector<glm::vec3> colors(positions.size(), glm::vec3(1.0f));
    return Mesh(name, positions, uvs, colors, indices, normals);
}

Material AssetPrefabs::solidColor(const std::string& name) {
    return Material(name,
                    GBUFFER_SHADER,
                    glm::vec4(1.0f),
                    EMPTY_TEXTURE,
                    EMPTY_TEXTURE,
                    EMPTY_TEXTURE,
                    EMPTY_TEXTURE,
                    EMPTY_TEXTURE,
                    0.0f,
                    1.0f,
                    1.0f,
                    glm::vec2(1.0f),
                    glm::vec2(0.0f),
                    false);
}
