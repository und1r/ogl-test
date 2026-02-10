#define GLFW_INCLUDE_NONE
#include "ShaderProgram.h"
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

const char *VERTEX_SHADER_SOURCE = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec3 aNormal;
    layout (location = 2) in vec2 aTexCoords;
    layout (location = 3) in ivec4 boneIds;
    layout (location = 4) in vec4 weights;

    out vec3 FragPos;
    out vec3 Normal;
    out vec2 TexCoords;
    out vec4 FragPosLightSpace;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    uniform mat3 normalMatrix;
    uniform mat4 lightSpaceMatrix;

    const int MAX_BONES = 100;
    const int MAX_BONE_INFLUENCE = 4;
    uniform mat4 finalBonesMatrices[MAX_BONES];
    uniform bool useAnimation;

    void main()
    {
        vec4 totalPosition = vec4(0.0f);
        vec3 totalNormal = vec3(0.0f);

        if (useAnimation) {
            for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++)
            {
                if(boneIds[i] == -1)
                    continue;

                if(boneIds[i] >= MAX_BONES)
                {
                    totalPosition = vec4(aPos,1.0f);
                    break;
                }

                vec4 localPosition = finalBonesMatrices[boneIds[i]] * vec4(aPos,1.0f);
                totalPosition += localPosition * weights[i];

                vec3 localNormal = mat3(finalBonesMatrices[boneIds[i]]) * aNormal;
                totalNormal += localNormal * weights[i];
            }
        } else {
            totalPosition = vec4(aPos, 1.0f);
            totalNormal = aNormal;
        }

        FragPos = vec3(model * totalPosition);
        Normal = normalMatrix * totalNormal;
        TexCoords = aTexCoords;
        FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);

        gl_Position = projection * view * vec4(FragPos, 1.0);
    }
)";

const char *FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core
    out vec4 FragColor;

    in vec3 Normal;
    in vec3 FragPos;
    in vec2 TexCoords;
    in vec4 FragPosLightSpace;

    uniform vec3 lightPos;
    uniform vec3 viewPos;
    uniform vec3 lightColor;
    uniform vec3 objectColor;
    uniform bool useTexture;

    uniform sampler2D texture_diffuse1;
    uniform sampler2D shadowMap;

    float shadowCalculation(vec4 fragPosLightSpace)
    {
        vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
        projCoords = projCoords * 0.5 + 0.5;
        if(projCoords.z > 1.0) return 0.0;
        float closestDepth = texture(shadowMap, projCoords.xy).r;
        float currentDepth = projCoords.z;
        float bias = 0.005;
        float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
        return shadow;
    }

    void main()
    {
        vec4 baseColor;
        if (useTexture) {
            baseColor = texture(texture_diffuse1, TexCoords);
            if(baseColor.a < 0.1) discard;
        } else {
            baseColor = vec4(objectColor, 1.0);
        }

        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);

        float distance    = length(lightPos - FragPos);
        float constant    = 1.0;
        float linear      = 0.007;
        float quadratic   = 0.00017;
        float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance));

        float ambientStrength = 0.35;
        vec3 ambient = ambientStrength * lightColor;

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;

        float shadow = shadowCalculation(FragPosLightSpace);
        float shadowStrength = 0.9;
        vec3 lighting = (ambient + (1.0 - shadow * shadowStrength) * (diffuse + specular));

        vec3 result = lighting * baseColor.rgb;
        FragColor = vec4(result, 1.0);
    }
)";

void checkCompileErrors(unsigned int shader_id, std::string type) {
    int success;
    char info_log[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader_id, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader_id, 1024, NULL, info_log);
            std::cerr << "ERROR::SHADER_COMPILATION_ERROR of type: " << type
                      << "\n"
                      << info_log << std::endl;
        }
    } else {
        glGetProgramiv(shader_id, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader_id, 1024, NULL, info_log);
            std::cerr << "ERROR::PROGRAM_LINKING_ERROR of type: " << type
                      << "\n"
                      << info_log << std::endl;
        }
    }
}

const char *DEPTH_VERTEX_SHADER_SOURCE = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 lightSpaceMatrix;
    uniform mat4 model;
    void main()
    {
        gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    }
)";

const char *DEPTH_FRAGMENT_SHADER_SOURCE = R"(
    #version 330 core
    void main() { }
)";

ShaderProgram createShaderProgram() {
    ShaderProgram program;
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &VERTEX_SHADER_SOURCE, NULL);
    glCompileShader(vertex_shader);
    checkCompileErrors(vertex_shader, "VERTEX");

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &FRAGMENT_SHADER_SOURCE, NULL);
    glCompileShader(fragment_shader);
    checkCompileErrors(fragment_shader, "FRAGMENT");

    program.id = glCreateProgram();
    glAttachShader(program.id, vertex_shader);
    glAttachShader(program.id, fragment_shader);
    glLinkProgram(program.id);
    checkCompileErrors(program.id, "PROGRAM");

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

ShaderProgram createDepthShaderProgram() {
    ShaderProgram program;
    unsigned int vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &DEPTH_VERTEX_SHADER_SOURCE, NULL);
    glCompileShader(vertex_shader);
    checkCompileErrors(vertex_shader, "VERTEX");

    unsigned int fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &DEPTH_FRAGMENT_SHADER_SOURCE, NULL);
    glCompileShader(fragment_shader);
    checkCompileErrors(fragment_shader, "FRAGMENT");

    program.id = glCreateProgram();
    glAttachShader(program.id, vertex_shader);
    glAttachShader(program.id, fragment_shader);
    glLinkProgram(program.id);
    checkCompileErrors(program.id, "PROGRAM");

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

void useShaderProgram(const ShaderProgram &program) {
    glUseProgram(program.id);
}
void setShaderMat4(const ShaderProgram &program, const std::string &name,
                   const glm::mat4 &mat) {
    glUniformMatrix4fv(glGetUniformLocation(program.id, name.c_str()), 1,
                       GL_FALSE, glm::value_ptr(mat));
}
void setShaderVec3(const ShaderProgram &program, const std::string &name,
                   const glm::vec3 &vec) {
    glUniform3fv(glGetUniformLocation(program.id, name.c_str()), 1,
                 glm::value_ptr(vec));
}
void setShaderVec3(const ShaderProgram &program, const std::string &name,
                   float x, float y, float z) {
    glUniform3f(glGetUniformLocation(program.id, name.c_str()), x, y, z);
}
void setShaderInt(const ShaderProgram &program, const std::string &name,
                  int value) {
    glUniform1i(glGetUniformLocation(program.id, name.c_str()), value);
}
void setShaderBool(const ShaderProgram &program, const std::string &name,
                   bool value) {
    glUniform1i(glGetUniformLocation(program.id, name.c_str()), (int)value);
}
