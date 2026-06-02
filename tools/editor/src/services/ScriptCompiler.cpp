#include "ScriptCompiler.hpp"
#include <log4cxx/logger.h>
#include <subprocess/subprocess.hpp>

namespace {
    const log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("ScriptCompiler");

    bool isStale(const std::filesystem::path& dylib, const std::filesystem::path& scriptsDir) {
        if (!std::filesystem::exists(dylib)) return true;
        auto dylibTime = std::filesystem::last_write_time(dylib);
        for (const auto& entry: std::filesystem::directory_iterator(scriptsDir)) {
            if (entry.path().extension() == ".cpp" && std::filesystem::last_write_time(entry.path()) > dylibTime)
                return true;
        }
        return false;
    }
} // namespace

std::filesystem::path ScriptCompiler::dylibPath(const std::filesystem::path& scriptsDir) {
    return scriptsDir / "scripts.dylib";
}

CompileResult ScriptCompiler::compile(const std::filesystem::path& scriptsDir) const {
    CompileResult result;

    std::vector<std::string> args = {"clang++",
                                     "-shared",
                                     "-fPIC",
                                     "-std=c++23",
                                     "-undefined",
                                     "dynamic_lookup",
                                     "-I",
                                     ENGINE_SRC_DIR,
                                     "-I",
                                     ENGINE_DEPS_INCLUDE_DIR,
                                     "-o",
                                     dylibPath(scriptsDir).string()};

    for (const auto& entry: std::filesystem::directory_iterator(scriptsDir)) {
        if (entry.path().extension() == ".cpp") args.push_back(entry.path().string());
    }

    auto proc = subprocess::Popen(args, subprocess::output{subprocess::PIPE}, subprocess::error{subprocess::PIPE});

    auto [out, err] = proc.communicate();
    result.output = std::string(out.buf.begin(), out.buf.end()) + std::string(err.buf.begin(), err.buf.end());
    result.success = (proc.retcode() == 0);

    if (result.success)
        LOG4CXX_INFO(logger, "Compiled scripts.dylib");
    else
        LOG4CXX_WARN(logger, "Failed to compile scripts.dylib\n" << result.output);

    return result;
}

std::optional<CompileResult> ScriptCompiler::compileStale(const std::filesystem::path& scriptsDir) const {
    if (!std::filesystem::exists(scriptsDir)) return std::nullopt;
    if (!isStale(dylibPath(scriptsDir), scriptsDir)) return std::nullopt;
    return compile(scriptsDir);
}
