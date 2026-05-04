#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include "data/assets/Mesh.hpp"

class ProceduralMeshGenerator {
public:
    /**
     * Generate a procedural mesh based on the name.
     * Name should start with _PROCEDURAL and include type and parameters.
     * Examples: "_PROCEDURAL_SPHERE_32"
     * @param name The procedural mesh name including type and parameters
     * @return Unique pointer to Mesh object
     */
    static std::unique_ptr<Mesh> generate(const std::string& name) {
        if (name.find("SPHERE") != std::string::npos) {
            size_t lastUnderscore = name.rfind('_');
            if (lastUnderscore != std::string::npos) {
                uint32_t resolution = std::stoul(name.substr(lastUnderscore + 1));
                return generateSphere(resolution, name);
            }
        }
        throw std::runtime_error("Unknown procedural mesh type: " + name);
    }

private:
    /**
     * Generate a UV sphere with unit radius.
     * @param resolution Number of segments (both latitude and longitude)
     * @param name The mesh name to assign
     * @return Unique pointer to Mesh object containing the sphere geometry
     */
    static std::unique_ptr<Mesh> generateSphere(uint32_t resolution, const std::string& name) {
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

        return std::make_unique<Mesh>(name, positions, uvs, colors, indices, normals);
    }
};
