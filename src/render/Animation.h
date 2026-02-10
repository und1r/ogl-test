#ifndef ANIMATION_H
#define ANIMATION_H

#include "Model.h" // For BoneInfo
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <string>
#include <vector>

struct KeyPosition {
    glm::vec3 position;
    float time_stamp;
};

struct KeyRotation {
    glm::quat orientation;
    float time_stamp;
};

struct KeyScale {
    glm::vec3 scale;
    float time_stamp;
};

// Represents the animation curves for a single bone
struct BoneAnimation {
    std::string name;
    int id;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale> scales;

    // Helpers to find the index of the keyframe just before the current time
    int getPositionIndex(float animation_time);
    int getRotationIndex(float animation_time);
    int getScaleIndex(float animation_time);
};

// Represents the hierarchy of bones (mirroring Assimp's node structure)
struct AssimpNodeData {
    glm::mat4 transformation;
    std::string name;
    int children_count;
    std::vector<AssimpNodeData> children;
};

// Holds the entire animation clip
struct Animation {
    float duration;
    int ticks_per_second;
    std::vector<BoneAnimation> bones; // Vector of all bones involved
    AssimpNodeData root_node;
    std::map<std::string, BoneInfo> bone_info_map; // Copy of model's bone info
};

// Holds the runtime state of the animation
struct Animator {
    std::vector<glm::mat4> final_bone_matrices;
    Animation *current_animation;
    float current_time;
    float delta_time;
};

// --- Functions ---

Animation loadAnimation(const std::string &animation_path, Model *model);
void updateAnimator(Animator &animator, float dt);
void playAnimation(Animator &animator, Animation *animation);

#endif
