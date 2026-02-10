#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <GLFW/glfw3.h>

struct Engine; // Forward declare Engine struct

void framebufferSizeCallback(GLFWwindow *window, int width, int height);
void mouseCallback(GLFWwindow *window, double xpos_in, double ypos_in);

#endif
