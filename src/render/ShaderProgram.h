#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <string>
#include <glm/glm.hpp>

struct ShaderProgram {
    unsigned int id;
};

// Lifecycle
ShaderProgram createShaderProgram();
ShaderProgram createDepthShaderProgram();
void useShaderProgram(const ShaderProgram& program);

// Uniforms
void setShaderMat4(const ShaderProgram& program, const std::string& name, const glm::mat4& mat);
void setShaderVec3(const ShaderProgram& program, const std::string& name, const glm::vec3& vec);
void setShaderVec3(const ShaderProgram& program, const std::string& name, float x, float y, float z);
void setShaderInt(const ShaderProgram& program, const std::string& name, int value);
void setShaderBool(const ShaderProgram& program, const std::string& name, bool value);

#endif
