#pragma once
#include <string>
#include <vector>

struct ShaderPipeline final {
    struct ShaderStage {
        std::string stageName{};
        std::vector<char> bytecode{};
    };

    std::string name{};
    ShaderStage vertexStage;
    ShaderStage fragmentStage;
};
