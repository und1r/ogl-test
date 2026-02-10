#ifndef MODEL_H
#define MODEL_H

#include "../math/GeometryUtils.h" // Vertex
#include "ShaderProgram.h"
#include <glm/glm.hpp>
#include <map>
#include <string>
#include <vector>

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

// Helper struct for Animation
struct BoneInfo {
    int id;           // Index in the final bone matrices array
    glm::mat4 offset; // Transforms from model space to bone space
};

struct Mesh {
    unsigned int vao;
    unsigned int vbo;
    unsigned int ebo;
    unsigned int index_count;

    std::vector<Texture> textures;
    glm::vec3 diffuse_color;

    // Keeping raw data for debugging or CPU-side physics if needed
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
};

struct Model {
    std::vector<Mesh> meshes;
    std::vector<Texture> loaded_textures;
    std::string directory;

    // Animation Data
    std::map<std::string, BoneInfo> bone_info_map; // Maps bone name to info
    int bone_counter = 0; // Tracks number of bones found
};

// Loads a model from a file path
Model loadModel(const std::string &path);

// Draws the model using the provided shader
void drawModel(const Model &model, const ShaderProgram &shader);

#endif
