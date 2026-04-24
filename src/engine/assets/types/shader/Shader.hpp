#pragma once
#include <string>
#include <vector>
#include "assets/types/base/Asset.hpp"

class Shader final : public Asset {
public:
    Shader(const std::string& name,
           std::vector<char> vertexBytecode,
           std::vector<char> fragmentBytecode,
           bool disableCull,
           bool disableDepthTest,
           bool disableDepthWrite,
           bool enableAlphaBlend) :
        Asset(name),
        vertexBytecode_(std::move(vertexBytecode)),
        fragmentBytecode_(std::move(fragmentBytecode)),
        disableCull(disableCull),
        disableDepthTest(disableDepthTest),
        disableDepthWrite(disableDepthWrite),
        enableAlphaBlend(enableAlphaBlend) {}

    const std::vector<char>& getVertexBytecode() const { return vertexBytecode_; }
    const std::vector<char>& getFragmentBytecode() const { return fragmentBytecode_; }
    bool cullDisabled() const { return disableCull; }
    bool depthTestDisabled() const { return disableDepthTest; }
    bool depthWriteDisabled() const { return disableDepthWrite; }
    bool alphaBlendEnabled() const { return enableAlphaBlend; }

private:
    std::vector<char> vertexBytecode_;
    std::vector<char> fragmentBytecode_;
    bool disableCull;
    bool disableDepthTest;
    bool disableDepthWrite;
    bool enableAlphaBlend;
};
