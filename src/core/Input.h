#ifndef INPUT_H
#define INPUT_H

#include "State.h"
#include <GLFW/glfw3.h>

void processInput(GLFWwindow *window, GameState &state);
void processPlayerKeyboard(SceneObject& player, GameState& state, CameraMovement direction, float delta_time);

#endif
