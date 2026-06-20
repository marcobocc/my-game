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
           bool tangentVertexLayout = false,
           bool depthBias = false,
           bool depthLessOrEqual = false,
           std::vector<char> computeBytecode = {},
           bool skinnedVertexLayout = false) :
        Asset(name),
        vertexBytecode(std::move(vertexBytecode)),
        fragmentBytecode(std::move(fragmentBytecode)),
        computeBytecode(std::move(computeBytecode)),
        disableCull(disableCull),
        disableDepthTest(disableDepthTest),
        disableDepthWrite(disableDepthWrite),
        enableAlphaBlend(enableAlphaBlend),
        noVertexInput(noVertexInput),
        lineTopology(lineTopology),
        positionColorVertexLayout(positionColorVertexLayout),
        tangentVertexLayout(tangentVertexLayout),
        depthBias(depthBias),
        depthLessOrEqual(depthLessOrEqual),
        skinnedVertexLayout(skinnedVertexLayout) {}

    std::vector<char> vertexBytecode;
    std::vector<char> fragmentBytecode;
    std::vector<char> computeBytecode; // non-empty for compute-only shaders
    bool disableCull;
    bool disableDepthTest;
    bool disableDepthWrite;
    bool enableAlphaBlend;
    bool noVertexInput;
    bool lineTopology;
    bool positionColorVertexLayout;
    bool tangentVertexLayout;
    bool depthBias;
    bool depthLessOrEqual;
    bool skinnedVertexLayout = false;
};
