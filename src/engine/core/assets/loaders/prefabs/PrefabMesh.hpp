#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "core/assets/PrefabNames.hpp"
#include "core/assets/types/MeshData.hpp"

class PrefabMesh {
public:
    static std::unique_ptr<MeshData> load(const std::string& name) {
        auto it = factories_.find(name);
        if (it != factories_.end()) return it->second();
        return nullptr;
    }

private:
    static std::unique_ptr<MeshData> makeCube();
    static std::unique_ptr<MeshData> makeTriangle();

    inline static const std::unordered_map<std::string, std::function<std::unique_ptr<MeshData>()>> factories_ = {
            {PRIMITIVE_GEOMETRY_CUBE, &PrefabMesh::makeCube}, {PRIMITIVE_GEOMETRY_TRIANGLE, &PrefabMesh::makeTriangle}};
};
