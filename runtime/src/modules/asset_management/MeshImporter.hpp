#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "asset_types/Mesh.hpp"

class VirtualFileSystem;

namespace importing {
    std::vector<std::unique_ptr<Mesh>> importMeshFile(const std::filesystem::path& filepath,
                                                      bool reverseWinding,
                                                      const std::string& namePrefix,
                                                      const VirtualFileSystem& vfs);
}
