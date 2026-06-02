#pragma once
#include <filesystem>
#include <optional>
#include <string>

struct CompileResult {
    bool success = false;
    std::string output; // combined stdout + stderr from the compiler
};

class ScriptCompiler {
public:
    // Compile all .cpp files in scriptsDir into a single scripts.dylib.
    CompileResult compile(const std::filesystem::path& scriptsDir) const;

    // Compile only if any .cpp is newer than scripts.dylib (or dylib is missing).
    // Returns empty optional if nothing was stale.
    std::optional<CompileResult> compileStale(const std::filesystem::path& scriptsDir) const;

    static std::filesystem::path dylibPath(const std::filesystem::path& scriptsDir);
};
