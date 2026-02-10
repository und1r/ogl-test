#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "render/Model.h"
#include <glm/glm.hpp>
#include <string>

#include <glm/gtc/quaternion.hpp> // Include for glm::quat

struct SceneObject {
    std::string name;
    Model model;
    glm::vec3 position;
    glm::quat orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Use a quaternion for rotation
    glm::vec3 scale;
    float y_velocity = 0.0f; // For gravity and jumping
    bool is_grounded = false; // To track if the object is on the ground
};

#endif
