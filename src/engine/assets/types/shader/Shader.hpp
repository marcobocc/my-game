#pragma once
#include <string>
#include "assets/types/base/Asset.hpp"


class Shader final : public Asset {
public:
    Shader(const std::string& name, std::vector<char> vertexBytecode, std::vector<char> fragmentBytecode) :
        Asset(name),
        vertexBytecode_(std::move(vertexBytecode)),
        fragmentBytecode_(std::move(fragmentBytecode)) {}

    const std::vector<char>& getVertexBytecode() const { return vertexBytecode_; }
    const std::vector<char>& getFragmentBytecode() const { return fragmentBytecode_; }

private:
    std::vector<char> vertexBytecode_;
    std::vector<char> fragmentBytecode_;
};
