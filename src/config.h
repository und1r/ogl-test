#ifndef CONFIG_H
#define CONFIG_H

namespace Config {
const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;
const char *const WINDOW_TITLE = "OpenGL Memeing";

// Global Lighting Colors
const float LIGHT_COLOR_R = 3.0f;
const float LIGHT_COLOR_G = 3.0f;
const float LIGHT_COLOR_B = 3.0f;

// Light Orbit Animation
const float LIGHT_ORBIT_RADIUS = 50.0f;
const float LIGHT_ORBIT_SPEED = 0.1f; // Radians per second

// Light Sphere Visualization
const float LIGHT_SPHERE_SCALE = 1.0f;

// Camera Settings
const float FIELD_OF_VIEW = 90.0f;
const float MOUSE_SENSITIVITY = 0.05f;
const float MOVEMENT_SPEED = 20.0f;

// Physics Settings
const float GRAVITY_STRENGTH = 9.81f;
const float PLAYER_RADIUS = 1.5f; // Radius of the player's collision sphere
const float JUMP_STRENGTH = 5.0f; // Initial vertical velocity for a jump

// Dynamic Light Position (for main light source)
const float DYNAMIC_LIGHT_POS_X = 0.0f;
const float DYNAMIC_LIGHT_POS_Y = 40.0f;
const float DYNAMIC_LIGHT_POS_Z = -60.0f;

// Starting Position
const float CAM_START_X = 0.0f;
const float CAM_START_Y = 100.0f;
const float CAM_START_Z = 10.0f;
} // namespace Config

#endif
