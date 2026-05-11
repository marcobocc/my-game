#pragma once
#include <filesystem>
#include <string>
#include "structs/assets/Mesh.hpp"

namespace importing {
    std::unique_ptr<Mesh>
    importObjFile(const std::filesystem::path& filepath, bool reverseWinding, const std::string& name);
}
