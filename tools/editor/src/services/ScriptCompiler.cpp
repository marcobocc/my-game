#include "ScriptCompiler.hpp"
#include <log4cxx/logger.h>
#include <subprocess/subprocess.hpp>

namespace {
    const log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ScriptCompiler");

    std::filesystem::path dylibPath(const std::filesystem::path& sourceFile) {
        return sourceFile.parent_path() / (sourceFile.stem().string() + ".dylib");
    }

    bool isStale(const std::filesystem::path& source, const std::filesystem::path& dylib) {
        if (!std::filesystem::exists(dylib)) return true;
        return std::filesystem::last_write_time(source) > std::filesystem::last_write_time(dylib);
    }
} // namespace

CompileResult ScriptCompiler::compile(const std::filesystem::path& sourceFile) const {
    CompileResult result;
    result.scriptName = sourceFile.stem().string();

    auto outPath = dylibPath(sourceFile);

    auto proc = subprocess::Popen({"clang++",
                                   "-shared",
                                   "-fPIC",
                                   "-std=c++23",
                                   sourceFile.string(),
                                   "-undefined",
                                   "dynamic_lookup",
                                   "-I",
                                   ENGINE_SRC_DIR,
                                   "-I",
                                   ENGINE_DEPS_INCLUDE_DIR,
                                   "-o",
                                   outPath.string()},
                                  subprocess::output{subprocess::PIPE},
                                  subprocess::error{subprocess::PIPE});

    auto [out, err] = proc.communicate();
    result.output = std::string(out.buf.begin(), out.buf.end()) + std::string(err.buf.begin(), err.buf.end());
    result.success = (proc.retcode() == 0);

    if (result.success)
        LOG4CXX_INFO(logger, "Compiled: " << result.scriptName);
    else
        LOG4CXX_WARN(logger, "Failed to compile: " << result.scriptName << "\n" << result.output);

    return result;
}

std::vector<CompileResult> ScriptCompiler::compileStale(const std::filesystem::path& scriptsDir) const {
    std::vector<CompileResult> results;

    if (!std::filesystem::exists(scriptsDir)) return results;

    for (const auto& entry: std::filesystem::directory_iterator(scriptsDir)) {
        if (entry.path().extension() != ".cpp") continue;
        if (isStale(entry.path(), dylibPath(entry.path()))) {
            results.push_back(compile(entry.path()));
        }
    }

    return results;
}
