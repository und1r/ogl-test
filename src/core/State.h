#ifndef STATE_H
#define STATE_H

#include "../core/Camera.h"
#include "../render/Animation.h" // NEW: Include Animation structs
#include "../scene/SceneObject.h"
#include <vector>

struct GameState {
    Camera camera;
    std::vector<SceneObject> scene_objects;

    // Input Handling
    float last_x;
    float last_y;
    bool first_mouse;
    float delta_time;
    float last_frame;
    int player_object_index = -1; // Index of the player SceneObject

    // Animation State
    Animation player_animation;
    Animator player_animator;
};

#endif
