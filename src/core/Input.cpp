#include "Input.h"
#include "../config.h"
#include "../scene/Scene.h"
#include "../scene/SceneObject.h"
#include "Camera.h"
#include <glm/gtc/quaternion.hpp>
#include <iostream>

void processPlayerKeyboard(SceneObject &player, GameState &state,
                           CameraMovement direction, float delta_time) {
    float move_speed = Config::MOVEMENT_SPEED;
    float velocity = move_speed * delta_time;

    // --- ROBUST FIX ---
    // Instead of relying on camera.front (which might be desynced),
    // we calculate "Forward" by drawing a line from the Camera to the Player.

    // 1. Get vector from Camera to Player (The direction we are looking)
    glm::vec3 view_direction = player.position - state.camera.position;

    // 2. Flatten it (Ignore Y so we don't fly into the ground)
    view_direction.y = 0.0f;

    // 3. Normalize to get the "Forward" vector
    if (glm::length(view_direction) > 0.001f)
        view_direction = glm::normalize(view_direction);
    else
        view_direction = glm::vec3(0.0f, 0.0f, -1.0f); // Fallback

    // 4. Calculate "Right" vector using Cross Product with World Up
    glm::vec3 right_direction =
        glm::cross(view_direction, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(right_direction) > 0.001f)
        right_direction = glm::normalize(right_direction);

    // 5. Apply Movement
    if (direction == FORWARD)
        player.position += view_direction * velocity;
    if (direction == BACKWARD)
        player.position -= view_direction * velocity;
    if (direction == LEFT)
        player.position -= right_direction * velocity;
    if (direction == RIGHT)
        player.position += right_direction * velocity;

    // Jumping
    if (direction == UP && player.is_grounded) {
        player.y_velocity = Config::JUMP_STRENGTH;
        player.is_grounded = false;
    }
}

void processInput(GLFWwindow *window, GameState &state) {
    // Global Exits
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Mode Switching (Debounced)
    if (glfwGetKey(window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS &&
        state.last_frame - state.camera.last_tick_camera_toggle_time > 0.2f) {

        if (state.camera.current_mode == FREE_VIEW) {
            state.camera.current_mode = PLAYER_VIEW;
        } else {
            state.camera.current_mode = FREE_VIEW;
            state.first_mouse = true; // Reset mouse to prevent jumping
        }
        state.camera.last_tick_camera_toggle_time = state.last_frame;
    }

    // --- MODE: FREE VIEW (Ghost/Spectator) ---
    // Controls: WASD moves the CAMERA directly. Physics ignored.
    if (state.camera.current_mode == FREE_VIEW) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            processCameraKeyboard(state.camera, FORWARD, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            processCameraKeyboard(state.camera, BACKWARD, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            processCameraKeyboard(state.camera, LEFT, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            processCameraKeyboard(state.camera, RIGHT, state.delta_time);
    }

    // --- MODE: PLAYER VIEW (Third Person) ---
    // Controls: WASD moves the PLAYER model. Camera orbits automatically.
    else if (state.camera.current_mode == PLAYER_VIEW &&
             state.player_object_index != -1) {
        SceneObject &player = state.scene_objects[state.player_object_index];

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            processPlayerKeyboard(player, state, FORWARD, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            processPlayerKeyboard(player, state, BACKWARD, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            processPlayerKeyboard(player, state, LEFT, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            processPlayerKeyboard(player, state, RIGHT, state.delta_time);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            processPlayerKeyboard(player, state, UP, state.delta_time);
    }
}
