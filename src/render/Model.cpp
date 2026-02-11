#include "Model.h"

// Define STB_IMAGE_IMPLEMENTATION only here
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

// --- FIX START ---
// The order matters! GLAD must be included before GLFW.
// Additionally, defining GLFW_INCLUDE_NONE prevents GLFW from loading
// the standard OpenGL header, effectively "bulletproofing" this file.
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
// --- FIX END ---

// --- Internal Helpers ---

// Helper to set default values for new vertices
void resetVertexBoneData(Vertex &vertex) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        vertex.bone_ids[i] = -1;
        vertex.weights[i] = 0.0f;
    }
}

// Helper to add bone data to a vertex
void setVertexBoneData(Vertex &vertex, int bone_id, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
        if (vertex.bone_ids[i] < 0) {
            vertex.bone_ids[i] = bone_id;
            vertex.weights[i] = weight;
            return;
        }
    }
}

glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4 &from) {
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

void setupMeshBuffers(Mesh &mesh, const std::vector<Vertex> &vertices,
                      const std::vector<unsigned int> &indices) {
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);

    // 1. Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

    // 2. Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, normal));

    // 3. TexCoords
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, texCoords));

    // 4. Bone IDs (Ints)
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(Vertex),
                           (void *)offsetof(Vertex, bone_ids));

    // 5. Bone Weights (Floats)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, weights));

    glBindVertexArray(0);

    mesh.index_count = (unsigned int)indices.size();
}

unsigned int loadTexture(const char *path, const std::string &directory,
                         const aiScene *scene) {
    unsigned int texture_id;
    glGenTextures(1, &texture_id);

    int width, height, nr_components;
    unsigned char *data = nullptr;

    std::string filename = std::string(path);

    // CHECK FOR EMBEDDED TEXTURE
    if (filename[0] == '*') {
        int texture_index = std::stoi(filename.substr(1));
        if (texture_index < scene->mNumTextures) {
            aiTexture *embedded_texture = scene->mTextures[texture_index];
            if (embedded_texture->mHeight == 0) {
                data = stbi_load_from_memory(
                    reinterpret_cast<unsigned char *>(embedded_texture->pcData),
                    embedded_texture->mWidth, &width, &height, &nr_components,
                    0);
            } else {
                data = stbi_load_from_memory(
                    reinterpret_cast<unsigned char *>(embedded_texture->pcData),
                    embedded_texture->mWidth * embedded_texture->mHeight,
                    &width, &height, &nr_components, 0);
            }
        }
    } else {
        filename = directory + '/' + filename;
        data = stbi_load(filename.c_str(), &width, &height, &nr_components, 0);
    }

    if (data) {
        GLenum format;
        if (nr_components == 1)
            format = GL_RED;
        else if (nr_components == 3)
            format = GL_RGB;
        else if (nr_components == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, texture_id);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                     GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    } else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return texture_id;
}

std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type,
                                          std::string type_name, Model &model,
                                          const aiScene *scene) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);

        bool skip = false;
        for (unsigned int j = 0; j < model.loaded_textures.size(); j++) {
            if (std::strcmp(model.loaded_textures[j].path.data(),
                            str.C_Str()) == 0) {
                textures.push_back(model.loaded_textures[j]);
                skip = true;
                break;
            }
        }
        if (!skip) {
            Texture texture;
            texture.id = loadTexture(str.C_Str(), model.directory, scene);
            texture.type = type_name;
            texture.path = str.C_Str();
            textures.push_back(texture);
            model.loaded_textures.push_back(texture);
        }
    }
    return textures;
}

void extractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh,
                                  Model &model) {
    for (unsigned int bone_idx = 0; bone_idx < mesh->mNumBones; ++bone_idx) {
        int bone_id = -1;
        std::string bone_name = mesh->mBones[bone_idx]->mName.C_Str();

        if (model.bone_info_map.find(bone_name) == model.bone_info_map.end()) {
            BoneInfo new_bone_info;
            new_bone_info.id = model.bone_counter;
            new_bone_info.offset =
                aiMatrix4x4ToGlm(mesh->mBones[bone_idx]->mOffsetMatrix);

            model.bone_info_map[bone_name] = new_bone_info;
            bone_id = model.bone_counter;
            model.bone_counter++;
        } else {
            bone_id = model.bone_info_map[bone_name].id;
        }

        auto weights = mesh->mBones[bone_idx]->mWeights;
        int num_weights = mesh->mBones[bone_idx]->mNumWeights;

        for (int weight_idx = 0; weight_idx < num_weights; ++weight_idx) {
            int vertex_id = weights[weight_idx].mVertexId;
            float weight = weights[weight_idx].mWeight;

            if (vertex_id < vertices.size()) {
                setVertexBoneData(vertices[vertex_id], bone_id, weight);
            }
        }
    }
}

Mesh processMesh(aiMesh *mesh, const aiScene *scene, glm::mat4 transform,
                 Model &model) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    // Normal matrix for baking transforms into static meshes
    glm::mat3 normal_matrix =
        glm::transpose(glm::inverse(glm::mat3(transform)));

    bool has_bones = (mesh->mNumBones > 0);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        resetVertexBoneData(vertex);

        // --- CRITICAL FIX FOR RAGDOLL/SINKING ---
        // If the mesh has bones, we MUST NOT bake the static 'transform' into
        // the vertices. The Animation system expects vertices in Local (Bind)
        // Space.

        if (has_bones) {
            // SKELETAL MESH (Player): Keep vertices in Local Space
            vertex.position =
                glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                          mesh->mVertices[i].z);

            if (mesh->HasNormals()) {
                vertex.normal =
                    glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                              mesh->mNormals[i].z);
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        } else {
            // STATIC MESH (Castle): Bake the Transform into the vertices
            glm::vec4 pos =
                glm::vec4(mesh->mVertices[i].x, mesh->mVertices[i].y,
                          mesh->mVertices[i].z, 1.0f);
            glm::vec4 transformed_pos = transform * pos;
            vertex.position = glm::vec3(transformed_pos);

            if (mesh->HasNormals()) {
                glm::vec3 norm =
                    glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                              mesh->mNormals[i].z);
                vertex.normal = glm::normalize(normal_matrix * norm);
            } else {
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        if (mesh->mTextureCoords[0]) {
            vertex.texCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                         mesh->mTextureCoords[0][i].y);
        } else {
            vertex.texCoords = glm::vec2(0.0f, 0.0f);
        }

        vertices.push_back(vertex);
    }

    if (has_bones) {
        extractBoneWeightForVertices(vertices, mesh, model);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
    std::vector<Texture> diffuse_maps = loadMaterialTextures(
        material, aiTextureType_DIFFUSE, "texture_diffuse", model, scene);
    textures.insert(textures.end(), diffuse_maps.begin(), diffuse_maps.end());

    std::vector<Texture> base_color_maps = loadMaterialTextures(
        material, aiTextureType_BASE_COLOR, "texture_diffuse", model, scene);
    textures.insert(textures.end(), base_color_maps.begin(),
                    base_color_maps.end());

    aiColor4D color(0.5f, 0.5f, 0.5f, 1.0f);
    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color);

    Mesh new_mesh;
    new_mesh.textures = textures;
    new_mesh.diffuse_color = glm::vec3(color.r, color.g, color.b);
    new_mesh.vertices = vertices;
    new_mesh.indices = indices;

    setupMeshBuffers(new_mesh, vertices, indices);

    return new_mesh;
}

void processNode(aiNode *node, const aiScene *scene, glm::mat4 parent_transform,
                 Model &model) {
    glm::mat4 node_transform = aiMatrix4x4ToGlm(node->mTransformation);
    glm::mat4 global_transform = parent_transform * node_transform;

    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        model.meshes.push_back(
            processMesh(mesh, scene, global_transform, model));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, global_transform, model);
    }
}

// --- Public API ---

Model loadModel(const std::string &path) {
    Model model;
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals |
                  aiProcess_LimitBoneWeights);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        std::cout << "ERROR::ASSIMP::" << importer.GetErrorString()
                  << std::endl;
        return model;
    }
    model.directory = path.substr(0, path.find_last_of('/'));

    glm::mat4 identity = glm::mat4(1.0f);
    processNode(scene->mRootNode, scene, identity, model);

    return model;
}

void drawMesh(const Mesh &mesh, const ShaderProgram &shader) {
    if (mesh.textures.size() > 0) {
        setShaderBool(shader, "useTexture", true);
        unsigned int diffuse_nr = 1;
        for (unsigned int i = 0; i < mesh.textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = mesh.textures[i].type;
            if (name == "texture_diffuse")
                number = std::to_string(diffuse_nr++);

            setShaderInt(shader, (name + number).c_str(), i);
            glBindTexture(GL_TEXTURE_2D, mesh.textures[i].id);
        }
    } else {
        setShaderBool(shader, "useTexture", false);
        setShaderVec3(shader, "objectColor", mesh.diffuse_color);
    }

    glBindVertexArray(mesh.vao);
    glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
    glActiveTexture(GL_TEXTURE0);
}

void drawModel(const Model &model, const ShaderProgram &shader) {
    for (const auto &mesh : model.meshes) {
        drawMesh(mesh, shader);
    }
}
