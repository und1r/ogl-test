#ifndef ENGINE_H
#define ENGINE_H

#include <GLFW/glfw3.h>
#include "State.h"
#include "../render/ShaderProgram.h"
#include "../render/ShadowMap.h"
#include "../math/Octree.h" // For Collision::Octree

struct Engine {
    GLFWwindow* window;
    GameState state;
    ShaderProgram shader_program;
    ShaderProgram depth_shader_program;
    ShadowMap shadow_map;
    unsigned int light_sphere_vao;
    unsigned int light_sphere_vertex_count;
    Collision::Octree collision_octree; // For collision detection
};

Engine createEngine();
void runEngine(Engine& engine);
void cleanupEngine(Engine& engine);

#endif
