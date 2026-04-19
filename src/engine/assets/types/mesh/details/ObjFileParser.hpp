#pragma once
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include "VertexLayouts.hpp"

class ObjFileParser {
public:
    static std::pair<std::vector<Vertex_WithLayout_PositionUv>, std::vector<uint32_t>>
    parseFile(const std::filesystem::path& filepath) {
        ObjData data = loadObjData(filepath);
        return buildMesh(data);
    }

private:
    struct ObjData {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uvs;
        struct Index {
            int p = -1, uv = -1;
        };
        std::vector<std::vector<Index>> faces;
    };

    static ObjData loadObjData(const std::filesystem::path& path) {
        std::ifstream file(path);
        if (!file) throw std::runtime_error("Failed to open OBJ file: " + path.string());
        ObjData data;
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ss(line);
            std::string tag;
            ss >> tag;
            if (tag == "v") {
                glm::vec3 p;
                ss >> p.x >> p.y >> p.z;
                data.positions.push_back(p);
            } else if (tag == "vt") {
                glm::vec2 uv;
                ss >> uv.x >> uv.y;
                data.uvs.push_back(uv);
            } else if (tag == "f") {
                std::vector<ObjData::Index> face;
                std::string token;
                while (ss >> token) {
                    face.push_back(parseFaceVertex(token));
                }
                if (face.size() >= 3) data.faces.push_back(std::move(face));
            }
        }
        return data;
    }

    static ObjData::Index parseFaceVertex(const std::string& s) {
        ObjData::Index out;
        size_t first = s.find('/');
        if (first == std::string::npos) {
            out.p = std::stoi(s) - 1;
            return out;
        }
        out.p = std::stoi(s.substr(0, first)) - 1;
        size_t second = s.find('/', first + 1);
        if (second == std::string::npos) {
            if (first + 1 < s.size()) out.uv = std::stoi(s.substr(first + 1)) - 1;
            return out;
        }
        if (second > first + 1) out.uv = std::stoi(s.substr(first + 1, second - first - 1)) - 1;
        return out;
    }

    static std::pair<std::vector<Vertex_WithLayout_PositionUv>, std::vector<uint32_t>> buildMesh(const ObjData& data) {
        std::vector<Vertex_WithLayout_PositionUv> vertices;
        std::vector<uint32_t> indices;
        uint32_t idx = 0;
        for (const auto& face: data.faces) {
            for (size_t i = 1; i + 1 < face.size(); ++i) {
                const auto tri = {face[0], face[i], face[i + 1]};

                for (const auto& v: tri) {
                    glm::vec3 pos = data.positions[v.p];

                    glm::vec2 uv = (v.uv >= 0 && v.uv < (int) data.uvs.size()) ? data.uvs[v.uv] : glm::vec2(0.0f);

                    vertices.push_back({pos, uv});
                    indices.push_back(idx++);
                }
            }
        }
        return {vertices, indices};
    }
};
