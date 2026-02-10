#include "Camera.h"
#include "../config.h"
#include "../scene/Scene.h"
#include "../scene/SceneObject.h"
#include <cmath>

void updateCameraVectors(Camera &camera) {
    glm::vec3 front;
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);
    camera.right = glm::normalize(glm::cross(camera.front, camera.world_up));
    camera.up = glm::normalize(glm::cross(camera.right, camera.front));
}

Camera createCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) {
    Camera camera;
    camera.position = position;
    camera.world_up = up;
    camera.yaw = yaw;
    camera.pitch = pitch;
    camera.front = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.movement_speed = Config::MOVEMENT_SPEED; // Use config speed
    camera.mouse_sensitivity = Config::MOUSE_SENSITIVITY;
    camera.zoom = 45.0f;
    camera.last_tick_camera_toggle_time = 0.0f;

    updateCameraVectors(camera);
    return camera;
}

glm::mat4 getCameraViewMatrix(Camera &camera, const GameState &state) {

    // 1. PLAYER VIEW: Orbit Logic
    if (state.camera.current_mode == PLAYER_VIEW &&
        state.player_object_index != -1) {
        const SceneObject &player =
            state.scene_objects[state.player_object_index];
        glm::vec3 camera_target =
            player.position +
            glm::vec3(0.0f, 1.0f, 0.0f); // Look at head/shoulders

        // Calculate Orbit Position based on Yaw/Pitch
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, glm::radians(camera.yaw),
                               glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(camera.pitch),
                               glm::vec3(1.0f, 0.0f, 0.0f));

        glm::vec3 rotated_offset =
            glm::vec3(rotation * glm::vec4(camera.player_offset, 0.0f));
        glm::vec3 orbit_pos = camera_target - rotated_offset;

        // SYNC: Update the actual camera struct so the rest of the engine knows
        // where we are
        camera.position = orbit_pos;

        return glm::lookAt(camera.position, camera_target, camera.world_up);
    }

    // 2. FREE VIEW: Standard FPS Logic
    // In this mode, camera.position is modified by WASD in
    // processCameraKeyboard
    return glm::lookAt(camera.position, camera.position + camera.front,
                       camera.up);
}

void processCameraKeyboard(Camera &camera, CameraMovement direction,
                           float delta_time) {
    float velocity = camera.movement_speed * delta_time;
    // Standard FPS Noclip flight
    if (direction == FORWARD)
        camera.position += camera.front * velocity;
    if (direction == BACKWARD)
        camera.position -= camera.front * velocity;
    if (direction == LEFT)
        camera.position -= camera.right * velocity;
    if (direction == RIGHT)
        camera.position += camera.right * velocity;
    // Vertical flight
    // Note: Use World Up (0,1,0) for absolute Up/Down, or camera.up for
    // relative
    if (direction == UP) // We don't have an Enum for SPACE/CTRL flight, mapped
                         // SPACE to UP usually
        camera.position += camera.world_up * velocity;
    // For Down we need to handle it or map it, typically not strictly needed
    // for basic cam
}

void processCameraMouse(Camera &camera, float x_offset, float y_offset,
                        bool constrain_pitch) {
    x_offset *= camera.mouse_sensitivity;
    y_offset *= camera.mouse_sensitivity;

    camera.yaw += x_offset;
    camera.pitch += y_offset;

    if (constrain_pitch) {
        if (camera.pitch > 89.0f)
            camera.pitch = 89.0f;
        if (camera.pitch < -89.0f)
            camera.pitch = -89.0f;
    }

    updateCameraVectors(camera);
}
