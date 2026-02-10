#include "RenderUtils.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <cstddef> // offsetof
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../render/ShaderProgram.h"

namespace RenderUtils {

unsigned int createVaoFromVertices(const std::vector<Vertex> &vertices) {
    unsigned int vao, vbo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 &vertices[0], GL_STATIC_DRAW);

    // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);
    glEnableVertexAttribArray(0);
    // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, texCoords));
    glEnableVertexAttribArray(2);

    return vao;
}

unsigned int createAxisVAO() {
    std::vector<Vertex> axis;
    // X (Red)
    axis.push_back({glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec2(0)});
    axis.push_back({glm::vec3(100, 0, 0), glm::vec3(1, 0, 0), glm::vec2(0)});
    // Y (Green)
    axis.push_back({glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec2(0)});
    axis.push_back({glm::vec3(0, 100, 0), glm::vec3(0, 1, 0), glm::vec2(0)});
    // Z (Blue)
    axis.push_back({glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec2(0)});
    axis.push_back({glm::vec3(0, 0, 100), glm::vec3(0, 0, 1), glm::vec2(0)});
    return createVaoFromVertices(axis);
}

void drawAxis(unsigned int vao, const ShaderProgram& shader) {
    glm::mat4 model_axis = glm::mat4(1.0f);
    setShaderMat4(shader, "model", model_axis);
    setShaderBool(shader, "useTexture", false);

    glBindVertexArray(vao);
    
    // X - Red
    setShaderVec3(shader, "objectColor", 1.0f, 0.0f, 0.0f);
    glDrawArrays(GL_LINES, 0, 2);
    
    // Y - Green
    setShaderVec3(shader, "objectColor", 0.0f, 1.0f, 0.0f);
    glDrawArrays(GL_LINES, 2, 2);
    
    // Z - Blue
    setShaderVec3(shader, "objectColor", 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_LINES, 4, 2);
    
    glBindVertexArray(0);
}

} // namespace RenderUtils