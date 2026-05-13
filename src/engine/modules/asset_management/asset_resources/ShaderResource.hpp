#pragma once
#include <string>
#include <vector>
#include "CPUResource.hpp"

class ShaderResource final : public CPUResource {
public:
    ShaderResource(const std::string& name,
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
        CPUResource(name),
        vertexBytecode(std::move(vertexBytecode)),
        fragmentBytecode(std::move(fragmentBytecode)),
        disableCull(disableCull),
        disableDepthTest(disableDepthTest),
        disableDepthWrite(disableDepthWrite),
        enableAlphaBlend(enableAlphaBlend),
        noVertexInput(noVertexInput),
        lineTopology(lineTopology),
        positionColorVertexLayout(positionColorVertexLayout),
        tangentVertexLayout(tangentVertexLayout) {}

    std::vector<char> vertexBytecode;
    std::vector<char> fragmentBytecode;
    bool disableCull;
    bool disableDepthTest;
    bool disableDepthWrite;
    bool enableAlphaBlend;
    bool noVertexInput;
    bool lineTopology;
    bool positionColorVertexLayout;
    bool tangentVertexLayout;
};
