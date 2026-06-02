#pragma once
#include <filesystem>
#include <string>
#include <vector>

struct CompileResult {
    std::string scriptName;
    bool success = false;
    std::string output; // combined stdout + stderr from the compiler
};

class ScriptCompiler {
public:
    // Compile one .cpp file to a .dylib next to it.
    CompileResult compile(const std::filesystem::path& sourceFile) const;

    // Compile every .cpp in dir whose .dylib is missing or older than the source.
    std::vector<CompileResult> compileStale(const std::filesystem::path& scriptsDir) const;
};
