#pragma once
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "core/assets/types/ShaderPipeline.hpp"

class ShaderLoader {
public:
    explicit ShaderLoader(const std::filesystem::path& directory = "assets/shaders") : directory_(directory) {}

    std::unique_ptr<ShaderPipeline> load(const std::string& pipelineName) const {
        auto pipelineDir = directory_ / pipelineName;

        auto pipeline = std::make_unique<ShaderPipeline>();
        pipeline->name = pipelineName;

        auto vertexPath = findCompiledShader(pipelineDir, ".vert.spv");
        pipeline->vertexStage.stageName = vertexPath;
        pipeline->vertexStage.bytecode = readFile(vertexPath);

        auto fragmentPath = findCompiledShader(pipelineDir, ".frag.spv");
        pipeline->fragmentStage.stageName = fragmentPath;
        pipeline->fragmentStage.bytecode = readFile(fragmentPath);

        return pipeline;
    }

private:
    static std::filesystem::path findCompiledShader(const std::filesystem::path& pipelineDir,
                                                    const std::string& extension) {
        for (const auto& entry: std::filesystem::directory_iterator(pipelineDir)) {
            if (entry.path().string().ends_with(extension)) {
                return entry.path();
            }
        }
        throw std::runtime_error("No compiled shader found with extension " + extension + " in " +
                                 pipelineDir.string());
    }
    std::unique_ptr<ShaderPipeline>
    load(const std::string& name, const std::string& vertexStageFile, const std::string& fragmentStageFile) const {
        auto pipeline = std::make_unique<ShaderPipeline>();
        pipeline->name = name;
        pipeline->vertexStage.stageName = directory_ / vertexStageFile;
        pipeline->vertexStage.bytecode = readFile(pipeline->vertexStage.stageName);
        pipeline->fragmentStage.stageName = directory_ / fragmentStageFile;
        pipeline->fragmentStage.bytecode = readFile(pipeline->fragmentStage.stageName);
        return pipeline;
    }

    static std::vector<char> readFile(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file) throw std::runtime_error("Failed to open shader stage: " + path.string());
        size_t size = static_cast<size_t>(file.tellg());
        file.seekg(0);

        std::vector<char> buffer(size);
        file.read(buffer.data(), static_cast<std::streamsize>(size));
        return buffer;
    }

    std::filesystem::path directory_;
};
