#include <log4cxx/basicconfigurator.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/logger.h>
#include <log4cxx/patternlayout.h>
#include "../../engine/modules/core/GameWindow.hpp"
#include "EditorWiringContainer.hpp"

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
    EditorWiringContainer wiringContainer(window, projectRoot);
    EditorApp& editorApp = wiringContainer.editorApp();
    editorApp.run();

    return 0;
}
