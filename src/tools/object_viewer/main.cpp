#include "ObjectViewerApp.hpp"

int main() {
    constexpr unsigned int width = 1280;
    constexpr unsigned int height = 720;
    ObjectViewerApp app(width, height);
    app.run();
    return 0;
}
