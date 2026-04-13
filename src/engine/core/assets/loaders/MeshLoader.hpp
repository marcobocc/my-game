#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "core/assets/loaders/prefabs/PrefabMesh.hpp"
#include "core/assets/types/MeshData.hpp"

class MeshLoader {
public:
    explicit MeshLoader(const std::filesystem::path& assetsPath) : assetsPath_(assetsPath) {}

    std::unique_ptr<MeshData> load(const std::string& name) const {
        if (auto prefab = PrefabMesh::load(name)) return prefab;
        auto path = assetsPath_ / "meshes" / name;
        std::ifstream file(path);
        if (!file) throw std::runtime_error("Failed to open mesh: " + path.string());

        auto objData = parseObj(file);
        return convertToMesh(objData, name);
    }

private:
    struct ObjData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        struct Face {
            struct Vertex {
                int posIndex = -1;
                int uvIndex = -1;
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
            } else if (prefix == "vt") {
                glm::vec2 uv;
                iss >> uv.x >> uv.y;
                data.uvs.push_back(uv);
            } else if (prefix == "f") {
                ObjData::Face face;
                std::string vertStr;
                while (iss >> vertStr) {
                    ObjData::Face::Vertex v;
                    size_t pos = vertStr.find('/');
                    if (pos != std::string::npos) {
                        v.posIndex = std::stoi(vertStr.substr(0, pos)) - 1;
                        size_t pos2 = vertStr.find('/', pos + 1);
                        if (pos2 != std::string::npos && pos2 > pos + 1) {
                            v.uvIndex = std::stoi(vertStr.substr(pos + 1, pos2 - pos - 1)) - 1;
                        } else {
                            v.uvIndex = std::stoi(vertStr.substr(pos + 1)) - 1;
                        }
                    } else {
                        v.posIndex = std::stoi(vertStr) - 1;
                    }
                    face.vertices.push_back(v);
                }
                data.faces.push_back(face);
            }
        }
        return data;
    }

    std::unique_ptr<MeshData> convertToMesh(const ObjData& data, const std::string& name) const {
        std::vector<PositionUVVertex> vertices;
        std::vector<uint32_t> indices;
        uint32_t idx = 0;
        for (const auto& face: data.faces) {
            if (face.vertices.size() < 3) continue;
            for (size_t i = 1; i < face.vertices.size() - 1; ++i) {
                const auto &v0 = face.vertices[0], &v1 = face.vertices[i], &v2 = face.vertices[i + 1];
                for (auto v: {v0, v1, v2}) {
                    glm::vec3 pos = data.positions[v.posIndex];
                    glm::vec2 uv = (v.uvIndex >= 0 && v.uvIndex < (int)data.uvs.size()) ? data.uvs[v.uvIndex] : glm::vec2(0.0f);
                    vertices.push_back({pos, uv});
                    indices.push_back(idx++);
                }
            }
        }
        auto mesh = MeshFactory::createPositionUVMesh(name, vertices, indices);
        return std::make_unique<MeshData>(std::move(mesh));
    }

    std::filesystem::path assetsPath_;
};
