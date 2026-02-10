#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GameState;

enum CameraMovement { FORWARD, BACKWARD, LEFT, RIGHT, UP, DOWN };
enum CameraMode { FREE_VIEW, PLAYER_VIEW };

struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 world_up;
    float yaw;
    float pitch;
    float movement_speed;
    float mouse_sensitivity;
    float zoom;
    CameraMode current_mode = PLAYER_VIEW;
    glm::vec3 player_offset = glm::vec3(0.0f, 3.0f, 5.0f);
    float last_tick_camera_toggle_time = 0.0f;
};

Camera createCamera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f),
                    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f),
                    float yaw = -90.0f, float pitch = 0.0f);

glm::mat4 getCameraViewMatrix(Camera &camera, const GameState &state);

void processCameraKeyboard(Camera &camera, CameraMovement direction,
                           float delta_time);
void processCameraMouse(Camera &camera, float x_offset, float y_offset,
                        bool constrain_pitch = true);

#endif
