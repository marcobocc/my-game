#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "core/assets/types/Mesh.hpp"

class MeshLoader {
public:
    explicit MeshLoader(std::filesystem::path dir = "assets/meshes") : directory_(std::move(dir)) {}

    std::unique_ptr<Mesh> load(const std::string& name) const {
        auto path = directory_ / (name + ".obj");
        std::ifstream file(path);
        if (!file) throw std::runtime_error("Failed to open mesh: " + path.string());

        auto objData = parseObj(file);
        return convertToMesh(objData, name);
    }

private:
    struct ObjData {
        std::vector<glm::vec3> positions;
        struct Face {
            struct Vertex {
                int posIndex = -1;
            };
            std::vector<Vertex> vertices;
        };
        std::vector<Face> faces;
    };

    ObjData parseObj(std::ifstream& file) const {
        ObjData data;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;
            if (prefix == "v") {
                glm::vec3 p;
                iss >> p.x >> p.y >> p.z;
                data.positions.push_back(p);
            } else if (prefix == "f") {
                ObjData::Face face;
                std::string vertStr;
                while (iss >> vertStr) {
                    ObjData::Face::Vertex v;
                    v.posIndex = std::stoi(vertStr) - 1;
                    face.vertices.push_back(v);
                }
                data.faces.push_back(face);
            }
        }
        return data;
    }

    std::unique_ptr<Mesh> convertToMesh(const ObjData& data, const std::string& name) const {
        std::vector<PositionVertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t idx = 0;

        for (const auto& face: data.faces) {
            if (face.vertices.size() < 3) continue;
            for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
                const auto &v0 = face.vertices[0], &v1 = face.vertices[i], &v2 = face.vertices[i + 1];
                for (auto v: {v0, v1, v2}) {
                    vertices.push_back({data.positions[v.posIndex]});
                    indices.push_back(idx++);
                }
            }
        }

        auto mesh = MeshFactory::createPositionMesh(name, vertices, indices);
        return std::make_unique<Mesh>(std::move(mesh));
    }

    std::filesystem::path directory_;
};
