#ifndef RENDERER_H
#define RENDERER_H

#include <GLFW/glfw3.h>
#include "../core/State.h"
#include "ShaderProgram.h"
#include "ShadowMap.h"

void renderScene(GLFWwindow* window, GameState& state, ShaderProgram& shader_program, ShaderProgram& depth_shader_program, ShadowMap& shadow_map, unsigned int light_sphere_vao, unsigned int sphere_vertex_count);

#endif
