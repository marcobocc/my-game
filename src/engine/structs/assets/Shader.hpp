#pragma once
#include <string>
#include <vector>
#include "Asset.hpp"

class Shader final : public Asset {
public:
    Shader(const std::string& name,
           std::vector<char> vertexBytecode,
           std::vector<char> fragmentBytecode,
           bool disableCull,
           bool disableDepthTest,
           bool disableDepthWrite,
           bool enableAlphaBlend,
           bool noVertexInput = false,
           bool lineTopology = false,
           bool positionColorVertexLayout = false,
           bool tangentVertexLayout = false) :
        Asset(name),
        vertexBytecode_(std::move(vertexBytecode)),
        fragmentBytecode_(std::move(fragmentBytecode)),
        disableCull(disableCull),
        disableDepthTest(disableDepthTest),
        disableDepthWrite(disableDepthWrite),
        enableAlphaBlend(enableAlphaBlend),
        noVertexInput(noVertexInput),
        lineTopology(lineTopology),
        positionColorVertexLayout(positionColorVertexLayout),
        tangentVertexLayout(tangentVertexLayout) {}

    const std::vector<char>& getVertexBytecode() const { return vertexBytecode_; }
    const std::vector<char>& getFragmentBytecode() const { return fragmentBytecode_; }
    bool cullDisabled() const { return disableCull; }
    bool depthTestDisabled() const { return disableDepthTest; }
    bool depthWriteDisabled() const { return disableDepthWrite; }
    bool alphaBlendEnabled() const { return enableAlphaBlend; }
    bool hasNoVertexInput() const { return noVertexInput; }
    bool hasLineTopology() const { return lineTopology; }
    bool hasPositionColorVertexLayout() const { return positionColorVertexLayout; }
    bool hasTangentVertexLayout() const { return tangentVertexLayout; }

private:
    std::vector<char> vertexBytecode_;
    std::vector<char> fragmentBytecode_;
    bool disableCull;
    bool disableDepthTest;
    bool disableDepthWrite;
    bool enableAlphaBlend;
    bool noVertexInput;
    bool lineTopology;
    bool positionColorVertexLayout;
    bool tangentVertexLayout;
};
