#include "importing.hpp"
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <sstream>
#include <string>
#include <vector>

struct ParsedData {
    struct Index {
        int p = -1, uv = -1;
    };
    std::vector<glm::vec3> positions;
    std::vector<glm::vec2> uvs;
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
    std::vector<uint32_t> indices;
    uint32_t idx = 0;
    for (const auto& face: data.faces) {
        for (size_t i = 1; i + 1 < face.size(); ++i) {
            const auto triangle = {face[0], face[i], face[i + 1]};
            for (const auto& v: triangle) {
                positions.push_back(data.positions[v.p]);
                if (!data.uvs.empty()) {
                    uvs.push_back(v.uv >= 0 && v.uv < static_cast<int>(data.uvs.size()) ? data.uvs[v.uv]
                                                                                        : glm::vec2(0.0f));
                }
                indices.push_back(idx++);
            }
        }
    }
    if (reverseWinding) {
        for (size_t i = 0; i + 2 < indices.size(); i += 3)
            std::swap(indices[i + 1], indices[i + 2]);
    }
    return std::make_unique<Mesh>(name, std::move(positions), std::move(uvs), std::move(colors), std::move(indices));
}

namespace importing {
    std::unique_ptr<Mesh>
    importObjFile(const std::filesystem::path& filepath, bool reverseWinding, const std::string& name) {
        ParsedData data = parseObjData(filepath);
        return buildMesh(data, reverseWinding, name);
    }
} // namespace importing
