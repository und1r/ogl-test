#define GLFW_INCLUDE_NONE
#include "Engine.h"
#include <glad/gl.h>
#include <iostream>

#include <glm/gtc/quaternion.hpp>

#include "../config.h"
#include "../math/GeometryUtils.h"
#include "../render/Animation.h"
#include "../render/Renderer.h"
#include "../scene/Scene.h"
#include "../utils/RenderUtils.h"
#include "Callbacks.h"
#include "Input.h"

void initWindow(Engine &engine) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary_monitor);
    engine.window = glfwCreateWindow(
        mode->width, mode->height, Config::WINDOW_TITLE, primary_monitor, NULL);
    if (!engine.window) {
        std::cout << "Failed window" << std::endl;
        exit(-1);
    }
    glfwMakeContextCurrent(engine.window);
    glfwSetWindowUserPointer(engine.window, &engine);
    glfwSetFramebufferSizeCallback(engine.window, framebufferSizeCallback);
    glfwSetCursorPosCallback(engine.window, mouseCallback);
    glfwSetInputMode(engine.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void initGLAD() {
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress))
        exit(-1);
}

void initResources(Engine &engine) {
    engine.state.camera = createCamera(glm::vec3(
        Config::CAM_START_X, Config::CAM_START_Y, Config::CAM_START_Z));
    GLFWmonitor *primary_monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode *mode = glfwGetVideoMode(primary_monitor);
    engine.state.last_x = mode->width / 2.0f;
    engine.state.last_y = mode->height / 2.0f;
    engine.state.first_mouse = true;
    engine.state.last_frame = static_cast<float>(glfwGetTime());
    engine.state.delta_time = 0.0f;

    loadScene(engine.state);

    // --- ANIMATION INIT ---
    if (engine.state.player_object_index != -1) {
        SceneObject &player =
            engine.state.scene_objects[engine.state.player_object_index];
        // Load the animation from the player GLB file
        engine.state.player_animation =
            loadAnimation("../src/assets/player.glb", &player.model);
        playAnimation(engine.state.player_animator,
                      &engine.state.player_animation);
    }
    // ----------------------

    engine.collision_octree = Collision::createOctree(
        Collision::AABB{glm::vec3(-100), glm::vec3(100)}, 8, 8);
    if (!engine.state.scene_objects.empty()) {
        const SceneObject &castle = engine.state.scene_objects[0];
        for (const auto &mesh : castle.model.meshes) {
            for (size_t i = 0; i < mesh.indices.size(); i += 3) {
                Collision::Triangle tri;
                tri.v0 = mesh.vertices[mesh.indices[i + 0]].position;
                tri.v1 = mesh.vertices[mesh.indices[i + 1]].position;
                tri.v2 = mesh.vertices[mesh.indices[i + 2]].position;
                Collision::insertTriangleIntoOctree(
                    engine.collision_octree.root.get(), tri, 0, 8, 8);
            }
        }
    }
    engine.shader_program = createShaderProgram();
    engine.depth_shader_program = createDepthShaderProgram();
    engine.shadow_map = createShadowMap(1024, 1024);
    std::vector<Vertex> sphere_vertices =
        MathUtils::generateSphereVertices(1.0f, 30, 30);
    engine.light_sphere_vao =
        RenderUtils::createVaoFromVertices(sphere_vertices);
    engine.light_sphere_vertex_count = sphere_vertices.size();
    glEnable(GL_DEPTH_TEST);
}

Engine createEngine() {
    glfwInit();
    Engine engine;
    initWindow(engine);
    initGLAD();
    initResources(engine);
    return engine;
}

void runEngine(Engine &engine) {
    while (!glfwWindowShouldClose(engine.window)) {
        float current_frame = static_cast<float>(glfwGetTime());
        engine.state.delta_time = current_frame - engine.state.last_frame;
        engine.state.last_frame = current_frame;

        // --- ANIMATION UPDATE ---
        updateAnimator(engine.state.player_animator, engine.state.delta_time);
        // ------------------------

        // --- PHYSICS & COLLISION ---
        if (engine.state.player_object_index != -1 &&
            engine.state.camera.current_mode == PLAYER_VIEW) {
            SceneObject &player =
                engine.state.scene_objects[engine.state.player_object_index];
            player.is_grounded = false;

            player.y_velocity -=
                Config::GRAVITY_STRENGTH * engine.state.delta_time;
            player.position.y += player.y_velocity * engine.state.delta_time;

            glm::vec3 center =
                player.position + glm::vec3(0.0f, Config::PLAYER_RADIUS, 0.0f);
            Collision::AABB query = {
                center - glm::vec3(Config::PLAYER_RADIUS + 0.1f),
                center + glm::vec3(Config::PLAYER_RADIUS + 0.1f)};
            std::vector<Collision::Triangle> tris;
            Collision::getTrianglesFromOctree(
                engine.collision_octree.root.get(), query, tris);

            glm::vec3 final_normal(0.0f);
            float max_depth = 0.0f;
            bool hit = false;

            for (const auto &tri : tris) {
                glm::vec3 norm;
                float depth;
                if (MathUtils::checkSphereTriangleCollision(
                        center, Config::PLAYER_RADIUS, tri, norm, depth)) {
                    if (depth > max_depth) {
                        max_depth = depth;
                        final_normal = norm;
                        hit = true;
                    }
                }
            }

            if (hit) {
                player.position += final_normal * max_depth;
                if (final_normal.y > 0.5f) {
                    player.y_velocity = 0.0f;
                    player.is_grounded = true;
                } else if (final_normal.y < 0.1f) {
                    player.y_velocity = 0.0f;
                }
            }
        }

        // --- ROTATION SYNC ---
        if (engine.state.player_object_index != -1 &&
            engine.state.camera.current_mode == PLAYER_VIEW) {
            float yaw = engine.state.camera.yaw;

            // Adjust this offset (-90, +90, 0, 180) to align model back to
            // camera
            glm::quat yaw_rotation =
                glm::angleAxis(glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));

            engine.state.scene_objects[engine.state.player_object_index]
                .orientation = yaw_rotation;
        }

        processInput(engine.window, engine.state);
        renderScene(engine.window, engine.state, engine.shader_program,
                    engine.depth_shader_program, engine.shadow_map,
                    engine.light_sphere_vao, engine.light_sphere_vertex_count);
        glfwSwapBuffers(engine.window);
        glfwPollEvents();
    }
}

void cleanupEngine(Engine &engine) { glfwTerminate(); }
