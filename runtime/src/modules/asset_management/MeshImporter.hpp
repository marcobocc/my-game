#pragma once
#include <filesystem>
#include <string>
#include "asset_types/Mesh.hpp"

class VirtualFileSystem;

namespace importing {
    std::unique_ptr<Mesh> importObjFile(const std::filesystem::path& filepath,
                                        bool reverseWinding,
                                        const std::string& name,
                                        const VirtualFileSystem& vfs);
}
