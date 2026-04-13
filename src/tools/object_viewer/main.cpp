#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include "ObjectViewerApp.hpp"

int main(int argc, char* argv[]) {
    const log4cxx::LayoutPtr layout(new log4cxx::PatternLayout("%d [%t] %-5p %c (%F:%L) - %m%n"));
    const log4cxx::AppenderPtr console(new log4cxx::ConsoleAppender(layout));
    log4cxx::LoggerPtr logger(log4cxx::Logger::getRootLogger());
    log4cxx::Logger::getRootLogger()->addAppender(console);

    std::filesystem::path assetsPath;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--assets_path" && i + 1 < argc) {
            assetsPath = argv[i + 1];
            i++;
        }
    }
    if (assetsPath.empty()) {
        throw std::runtime_error("Assets path not provided. Use --assets_path <path> to specify the path to assets.");
    }

    constexpr unsigned int width = 1280;
    constexpr unsigned int height = 720;
    ObjectViewerApp app(width, height, assetsPath);
    app.run();
    return 0;
}
