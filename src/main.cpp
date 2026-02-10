#define GLFW_INCLUDE_NONE // Must be defined before GLFW/glfw3.h
#include "core/Engine.h"

int main() {
    Engine engine = createEngine();
    runEngine(engine);
    cleanupEngine(engine);
    return 0;
}
