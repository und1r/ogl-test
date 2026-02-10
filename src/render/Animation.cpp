#include "Animation.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <iostream>

// Helpers for Assimp -> GLM conversion
static glm::mat4 castMatrix(const aiMatrix4x4 &from) {
    glm::mat4 to;
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;
    return to;
}

// --- BoneAnimation Implementation ---

int BoneAnimation::getPositionIndex(float animation_time) {
    for (int i = 0; i < positions.size() - 1; ++i) {
        if (animation_time < positions[i + 1].time_stamp)
            return i;
    }
    return 0;
}

int BoneAnimation::getRotationIndex(float animation_time) {
    for (int i = 0; i < rotations.size() - 1; ++i) {
        if (animation_time < rotations[i + 1].time_stamp)
            return i;
    }
    return 0;
}

int BoneAnimation::getScaleIndex(float animation_time) {
    for (int i = 0; i < scales.size() - 1; ++i) {
        if (animation_time < scales[i + 1].time_stamp)
            return i;
    }
    return 0;
}

float getScaleFactor(float last_time_stamp, float next_time_stamp,
                     float animation_time) {
    float scale_factor = 0.0f;
    float mid_way_length = animation_time - last_time_stamp;
    float frames_diff = next_time_stamp - last_time_stamp;
    scale_factor = mid_way_length / frames_diff;
    return scale_factor;
}

glm::mat4 interpolatePosition(BoneAnimation &bone, float animation_time) {
    if (bone.positions.size() == 1)
        return glm::translate(glm::mat4(1.0f), bone.positions[0].position);

    int p0_index = bone.getPositionIndex(animation_time);
    int p1_index = p0_index + 1;
    float scale_factor =
        getScaleFactor(bone.positions[p0_index].time_stamp,
                       bone.positions[p1_index].time_stamp, animation_time);
    glm::vec3 final_position =
        glm::mix(bone.positions[p0_index].position,
                 bone.positions[p1_index].position, scale_factor);
    return glm::translate(glm::mat4(1.0f), final_position);
}

glm::mat4 interpolateRotation(BoneAnimation &bone, float animation_time) {
    if (bone.rotations.size() == 1) {
        auto rotation = glm::normalize(bone.rotations[0].orientation);
        return glm::mat4_cast(rotation);
    }

    int p0_index = bone.getRotationIndex(animation_time);
    int p1_index = p0_index + 1;
    float scale_factor =
        getScaleFactor(bone.rotations[p0_index].time_stamp,
                       bone.rotations[p1_index].time_stamp, animation_time);
    glm::quat final_rotation =
        glm::slerp(bone.rotations[p0_index].orientation,
                   bone.rotations[p1_index].orientation, scale_factor);
    final_rotation = glm::normalize(final_rotation);
    return glm::mat4_cast(final_rotation);
}

glm::mat4 interpolateScaling(BoneAnimation &bone, float animation_time) {
    if (bone.scales.size() == 1)
        return glm::scale(glm::mat4(1.0f), bone.scales[0].scale);

    int p0_index = bone.getScaleIndex(animation_time);
    int p1_index = p0_index + 1;
    float scale_factor =
        getScaleFactor(bone.scales[p0_index].time_stamp,
                       bone.scales[p1_index].time_stamp, animation_time);
    glm::vec3 final_scale = glm::mix(bone.scales[p0_index].scale,
                                     bone.scales[p1_index].scale, scale_factor);
    return glm::scale(glm::mat4(1.0f), final_scale);
}

// --- Loading Logic ---

void readHeirarchyData(AssimpNodeData &dest, const aiNode *src) {
    dest.name = src->mName.data;
    dest.transformation = castMatrix(src->mTransformation);
    dest.children_count = src->mNumChildren;

    for (int i = 0; i < src->mNumChildren; i++) {
        AssimpNodeData new_data;
        readHeirarchyData(new_data, src->mChildren[i]);
        dest.children.push_back(new_data);
    }
}

Animation loadAnimation(const std::string &animation_path, Model *model) {
    Animation animation;
    animation.bone_info_map = model->bone_info_map; // Copy bone info from model

    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(animation_path, aiProcess_Triangulate);

    // Check if scene has animations
    if (!scene || !scene->mRootNode || scene->mNumAnimations == 0) {
        std::cout << "Animation Load Error: No animations found in "
                  << animation_path << std::endl;
        return animation;
    }

    auto anim = scene->mAnimations[0]; // Load the first animation
    animation.duration = anim->mDuration;
    animation.ticks_per_second = anim->mTicksPerSecond;

    readHeirarchyData(animation.root_node, scene->mRootNode);

    // Read channels (Bone Animations)
    for (int i = 0; i < anim->mNumChannels; i++) {
        auto channel = anim->mChannels[i];
        BoneAnimation bone;
        bone.name = channel->mNodeName.data;

        if (animation.bone_info_map.find(bone.name) ==
            animation.bone_info_map.end()) {
            bone.id = -1;
        } else {
            bone.id = animation.bone_info_map[bone.name].id;
        }

        // Key positions
        for (int j = 0; j < channel->mNumPositionKeys; j++) {
            aiVector3D ai_pos = channel->mPositionKeys[j].mValue;
            float time_stamp = channel->mPositionKeys[j].mTime;
            bone.positions.push_back(
                {glm::vec3(ai_pos.x, ai_pos.y, ai_pos.z), time_stamp});
        }

        // Key rotations
        for (int j = 0; j < channel->mNumRotationKeys; j++) {
            aiQuaternion ai_ori = channel->mRotationKeys[j].mValue;
            float time_stamp = channel->mRotationKeys[j].mTime;
            bone.rotations.push_back(
                {glm::quat(ai_ori.w, ai_ori.x, ai_ori.y, ai_ori.z),
                 time_stamp});
        }

        // Key scalings
        for (int j = 0; j < channel->mNumScalingKeys; j++) {
            aiVector3D ai_scale = channel->mScalingKeys[j].mValue;
            float time_stamp = channel->mScalingKeys[j].mTime;
            bone.scales.push_back(
                {glm::vec3(ai_scale.x, ai_scale.y, ai_scale.z), time_stamp});
        }

        animation.bones.push_back(bone);
    }

    return animation;
}

// --- Animator Logic ---

void playAnimation(Animator &animator, Animation *animation) {
    animator.current_animation = animation;
    animator.current_time = 0.0f;
    animator.final_bone_matrices.clear();
    animator.final_bone_matrices.resize(100, glm::mat4(1.0f)); // Max 100 bones
}

BoneAnimation *findBoneAnimation(Animation *animation,
                                 const std::string &node_name) {
    for (auto &bone : animation->bones) {
        if (bone.name == node_name) {
            return &bone;
        }
    }
    return nullptr;
}

void calculateBoneTransform(Animator &animator, const AssimpNodeData *node,
                            glm::mat4 parent_transform) {
    std::string node_name = node->name;
    glm::mat4 node_transform = node->transformation;

    BoneAnimation *bone =
        findBoneAnimation(animator.current_animation, node_name);

    if (bone) {
        glm::mat4 trans = interpolatePosition(*bone, animator.current_time);
        glm::mat4 rot = interpolateRotation(*bone, animator.current_time);
        glm::mat4 scale = interpolateScaling(*bone, animator.current_time);
        node_transform = trans * rot * scale;
    }

    glm::mat4 global_transformation = parent_transform * node_transform;

    auto &bone_info_map = animator.current_animation->bone_info_map;
    if (bone_info_map.find(node_name) != bone_info_map.end()) {
        int index = bone_info_map[node_name].id;
        glm::mat4 offset = bone_info_map[node_name].offset;

        if (index < animator.final_bone_matrices.size())
            animator.final_bone_matrices[index] =
                global_transformation * offset;
    }

    for (int i = 0; i < node->children_count; i++) {
        calculateBoneTransform(animator, &node->children[i],
                               global_transformation);
    }
}

void updateAnimator(Animator &animator, float dt) {
    if (!animator.current_animation)
        return;

    animator.delta_time = dt;
    animator.current_time += animator.current_animation->ticks_per_second * dt;

    // Loop animation
    animator.current_time =
        fmod(animator.current_time, animator.current_animation->duration);

    calculateBoneTransform(animator, &animator.current_animation->root_node,
                           glm::mat4(1.0f));
}
