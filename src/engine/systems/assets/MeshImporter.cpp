#include "MeshImporter.hpp"
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <vector>

struct ParsedData {
    struct Index {
        int p = -1, uv = -1, n = -1;
    };
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<std::vector<Index>> faces;
};

ParsedData::Index parseFaceVertex(const std::string& s) {
    ParsedData::Index out;
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
    if (second + 1 < s.size()) out.n = std::stoi(s.substr(second + 1)) - 1;
    return out;
}

ParsedData parseObjData(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Failed to open OBJ file: " + path.string());
    ParsedData data;
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
        } else if (tag == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            data.normals.push_back(n);
        } else if (tag == "f") {
            std::vector<ParsedData::Index> face;
            std::string token;
            while (ss >> token) {
                face.push_back(parseFaceVertex(token));
            }
            if (face.size() >= 3) data.faces.push_back(std::move(face));
        }
    }
    return data;
}

std::unique_ptr<Mesh> buildMesh(const ParsedData& data, bool reverseWinding, const std::string& name) {
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> colors;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    uint32_t idx = 0;
    for (const auto& face: data.faces) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            const auto& v0 = face[0];
            const auto& v1 = face[i];
            const auto& v2 = face[i + 1];

            const glm::vec3& p0 = data.positions[v0.p];
            const glm::vec3& p1 = data.positions[v1.p];
            const glm::vec3& p2 = data.positions[v2.p];

            glm::vec3 faceNormal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            const auto processVertex = [&](const auto& v) {
                positions.push_back(data.positions[v.p]);

                if (!data.uvs.empty() && v.uv >= 0 && v.uv < static_cast<int>(data.uvs.size())) {
                    uvs.push_back(data.uvs[v.uv]);
                } else {
                    uvs.emplace_back(0.0f);
                }

                if (!data.normals.empty() && v.n >= 0 && v.n < static_cast<int>(data.normals.size())) {
                    normals.push_back(data.normals[v.n]);
                } else {
                    normals.push_back(faceNormal);
                }

                indices.push_back(idx++);
            };

            processVertex(v0);
            processVertex(v1);
            processVertex(v2);
        }
    }

    if (reverseWinding) {
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            std::swap(indices[i + 1], indices[i + 2]);
        }
    }

    return std::make_unique<Mesh>(
            name, std::move(positions), std::move(uvs), std::move(colors), std::move(indices), std::move(normals));
}

namespace importing {
    std::unique_ptr<Mesh>
    importObjFile(const std::filesystem::path& filepath, bool reverseWinding, const std::string& name) {
        ParsedData data = parseObjData(filepath);
        return buildMesh(data, reverseWinding, name);
    }
} // namespace importing
