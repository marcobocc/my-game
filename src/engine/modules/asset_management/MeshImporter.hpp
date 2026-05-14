#pragma once
#include <filesystem>
#include <string>
#include "asset_resources/MeshResource.hpp"

class VirtualFileSystem;

namespace importing {
    std::unique_ptr<MeshResource> importObjFile(const std::filesystem::path& filepath,
                                                bool reverseWinding,
                                                const std::string& name,
                                                const VirtualFileSystem& vfs);
}
