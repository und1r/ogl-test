#define GLFW_INCLUDE_NONE
#include "Renderer.h"
#include "../config.h"
#include "../math/GeometryUtils.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>
#include <string> // For std::to_string

void renderScene(GLFWwindow *window, GameState &state,
                 ShaderProgram &shader_program,
                 ShaderProgram &depth_shader_program, ShadowMap &shadow_map,
                 unsigned int light_sphere_vao,
                 unsigned int sphere_vertex_count) {
    // Animate light position
    float current_time = static_cast<float>(glfwGetTime());
    float light_orbit_x = Config::LIGHT_ORBIT_RADIUS *
                          glm::cos(current_time * Config::LIGHT_ORBIT_SPEED);
    float light_orbit_z = Config::LIGHT_ORBIT_RADIUS *
                          glm::sin(current_time * Config::LIGHT_ORBIT_SPEED);
    glm::vec3 lightPos =
        glm::vec3(light_orbit_x, Config::DYNAMIC_LIGHT_POS_Y, light_orbit_z);

    // 1. Render scene from light's point of view (to depth map)
    // ---------------------------------------------------------
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 1.0f, far_plane = 100.0f;
    lightProjection = glm::perspective(
        glm::radians(90.0f), (float)shadow_map.width / (float)shadow_map.height,
        near_plane, far_plane);
    lightView =
        glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
    lightSpaceMatrix = lightProjection * lightView;

    useShaderProgram(depth_shader_program);
    setShaderMat4(depth_shader_program, "lightSpaceMatrix", lightSpaceMatrix);

    glViewport(0, 0, shadow_map.width, shadow_map.height);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_map.depth_map_fbo);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Draw scene objects to depth map
    // Note: For accurate shadows, the depth shader ALSO needs animation
    // support! For now, shadows might look static/T-pose unless we update depth
    // shader too.
    for (const auto &object : state.scene_objects) {
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, object.position);
        model_matrix = model_matrix * glm::mat4_cast(object.orientation);
        model_matrix = glm::scale(model_matrix, object.scale);
        setShaderMat4(depth_shader_program, "model", model_matrix);

        // Quick hack: Pass identity matrices to depth shader if it has bone
        // uniforms, or just draw static for now to prevent crash. Ideally,
        // create a simplified "AnimationDepth" shader later.

        drawModel(object.model, depth_shader_program);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2. Render scene as normal with shadow map
    // -----------------------------------------
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    useShaderProgram(shader_program);

    glm::mat4 projection =
        glm::perspective(glm::radians(Config::FIELD_OF_VIEW),
                         (float)display_w / (float)display_h, 0.1f, 100.0f);
    setShaderMat4(shader_program, "projection", projection);

    glm::mat4 view = getCameraViewMatrix(state.camera, state);
    setShaderMat4(shader_program, "view", view);

    glm::vec3 actual_camera_pos;
    if (state.player_object_index != -1 &&
        state.camera.current_mode == PLAYER_VIEW) {
        // Camera position is already updated in getCameraViewMatrix for Player
        // Mode
        actual_camera_pos = state.camera.position;
    } else {
        actual_camera_pos = state.camera.position;
    }
    setShaderVec3(shader_program, "viewPos", actual_camera_pos);

    // Light uniforms
    setShaderVec3(shader_program, "lightPos", lightPos);
    setShaderVec3(shader_program, "lightColor", Config::LIGHT_COLOR_R,
                  Config::LIGHT_COLOR_G, Config::LIGHT_COLOR_B);
    setShaderMat4(shader_program, "lightSpaceMatrix", lightSpaceMatrix);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadow_map.depth_map_texture);
    setShaderInt(shader_program, "shadowMap", 1);

    // Draw scene objects
    for (const auto &object : state.scene_objects) {
        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, object.position);
        model_matrix = model_matrix * glm::mat4_cast(object.orientation);
        model_matrix = glm::scale(model_matrix, object.scale);
        setShaderMat4(shader_program, "model", model_matrix);

        glm::mat3 norm_mat = MathUtils::calculateNormalMatrix(model_matrix);
        glUniformMatrix3fv(
            glGetUniformLocation(shader_program.id, "normalMatrix"), 1,
            GL_FALSE, glm::value_ptr(norm_mat));

        // --- ANIMATION UNIFORMS ---
        if (object.name == "player") {
            setShaderBool(shader_program, "useAnimation", true);

            auto transforms = state.player_animator.final_bone_matrices;
            for (int i = 0; i < transforms.size(); ++i) {
                // Determine location manually or using string (string is slower
                // but safer for now)
                std::string name =
                    "finalBonesMatrices[" + std::to_string(i) + "]";
                setShaderMat4(shader_program, name, transforms[i]);
            }
        } else {
            setShaderBool(shader_program, "useAnimation", false);
        }
        // --------------------------

        drawModel(object.model, shader_program);
    }

    // Draw Light Sphere
    glm::mat4 model_light_sphere = glm::mat4(1.0f);
    model_light_sphere = glm::translate(model_light_sphere, lightPos);
    model_light_sphere =
        glm::scale(model_light_sphere, glm::vec3(Config::LIGHT_SPHERE_SCALE));
    setShaderMat4(shader_program, "model", model_light_sphere);
    setShaderVec3(shader_program, "objectColor", Config::LIGHT_COLOR_R,
                  Config::LIGHT_COLOR_G, Config::LIGHT_COLOR_B);
    setShaderBool(shader_program, "useTexture", false);
    setShaderBool(shader_program, "useAnimation",
                  false); // Important: Sphere is not animated
    glBindVertexArray(light_sphere_vao);
    glDrawArrays(GL_TRIANGLES, 0, sphere_vertex_count);
    glBindVertexArray(0);
}
