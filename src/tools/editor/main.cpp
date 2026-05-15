#include <filesystem>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include <vector>
#include "../../engine/modules/core/GameWindow.hpp"
#include "EditorWiringContainer.hpp"

std::vector<std::filesystem::path> createMountPaths(const std::filesystem::path& projectPath) {
    std::vector<std::filesystem::path> paths;
    std::filesystem::path builtinAssets = ENGINE_DATA_DIR;
    if (std::filesystem::exists(builtinAssets)) {
        paths.push_back(builtinAssets);
    }
    std::filesystem::path editorData = TOOL_DATA_DIR;
    if (std::filesystem::exists(editorData)) {
        paths.push_back(editorData);
    }
    if (!projectPath.empty()) {
        std::filesystem::path projectAssets = projectPath;
        if (std::filesystem::exists(projectAssets)) {
            paths.push_back(projectAssets);
        }
    }
    return paths;
}

int main(int argc, char* argv[]) {
    const log4cxx::LayoutPtr layout(new log4cxx::PatternLayout("%d [%t] %-5p %c (%F:%L) - %m%n"));
    const log4cxx::AppenderPtr console(new log4cxx::ConsoleAppender(layout));
    log4cxx::LoggerPtr logger(log4cxx::Logger::getRootLogger());
    log4cxx::Logger::getRootLogger()->addAppender(console);

    std::filesystem::path projectRoot;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--project" && i + 1 < argc) {
            projectRoot = std::filesystem::absolute(argv[i + 1]);
            i++;
        }
    }
    if (projectRoot.empty()) {
        throw std::runtime_error("Project root not provided. Use --project <path> to specify the project root.");
    }
    GameWindow window("Editor");
    auto mountPaths = createMountPaths(projectRoot);
    EditorWiringContainer wiringContainer(window, mountPaths, projectRoot);
    EditorApp& editorApp = wiringContainer.editorApp();
    editorApp.run();

    return 0;
}
