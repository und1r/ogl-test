#include "Callbacks.h"
#include "Engine.h" // Include Engine to access its members
#include "Camera.h" // For processCameraMouse

void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
    // Optionally, you might want to update engine.state.window_width/height here
}

void mouseCallback(GLFWwindow *window, double xpos_in, double ypos_in) {
    Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine) {
        float xpos = static_cast<float>(xpos_in);
        float ypos = static_cast<float>(ypos_in);

        if (engine->state.first_mouse) {
            engine->state.last_x = xpos;
            engine->state.last_y = ypos;
            engine->state.first_mouse = false;
        }

        float xoffset = engine->state.last_x - xpos; // Corrected for non-inverted X-axis
        float yoffset = ypos - engine->state.last_y; // Corrected for non-inverted Y-axis
        engine->state.last_x = xpos;
        engine->state.last_y = ypos;

        processCameraMouse(engine->state.camera, xoffset, yoffset, true);
    }
}
