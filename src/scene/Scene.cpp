#include "Scene.h"
#include "../config.h"       // For Config constants
#include "../render/Model.h" // For Model struct and loadModel function
#include <glm/glm.hpp>       // For glm::vec3
#include <iostream>

void loadScene(GameState &state) {
    // --- Load Static Environment (Castle) ---
    // We assume the castle is still the same model
    Model castle_model = loadModel("../src/assets/castle.gltf");

    state.scene_objects.push_back({
        "castle", castle_model, glm::vec3(0.0f, 0.0f, 0.0f), // Position
        glm::quat(1.0f, 0.0f, 0.0f,
                  0.0f), // Orientation (Identity / No rotation)
        glm::vec3(1.0f)  // Scale
    });

    // --- Load Player (New GLB Model) ---
    // GLB files often have embedded textures, which our new Model.cpp handles
    // automatically.
    Model player_model = loadModel("../src/assets/player.glb");

    // Standard GLB models usually face +Z or -Z.
    // If your character faces the wrong way on start, change this '0.0f' to
    // '180.0f'.
    glm::quat player_orientation =
        glm::angleAxis(glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 0.0f));

    state.scene_objects.push_back({
        "player", player_model,
        glm::vec3(Config::CAM_START_X, Config::CAM_START_Y,
                  Config::CAM_START_Z),
        player_orientation,
        glm::vec3(
            1.0f) // Start with 1.0f scale. Adjust if player is giant/tiny.
    });

    // Set the player index so the Engine knows which object to apply
    // physics/input to
    state.player_object_index =
        static_cast<int>(state.scene_objects.size() - 1);
}
