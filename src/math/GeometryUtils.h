#ifndef GEOMETRY_UTILS_H
#define GEOMETRY_UTILS_H

#include "../math/Octree.h" // Required for Collision::Triangle
#include <glm/glm.hpp>
#include <string>
#include <vector>

// Maximum number of bones that can influence a single vertex
#define MAX_BONE_INFLUENCE 4

// Structs as FirstCaps
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;

    // Animation Data
    // IDs of the bones that affect this vertex
    int bone_ids[MAX_BONE_INFLUENCE];
    // Weights determining how much each bone affects this vertex
    float weights[MAX_BONE_INFLUENCE];
};

// Functions wrapper
namespace MathUtils {

// Geometry Generation
std::vector<Vertex> generateSphereVertices(float radius, int stack_count,
                                           int sector_count);
std::vector<Vertex> generatePlaneVertices(float size);

// Helper to calculate normal matrix (for lighting calculations)
glm::mat3 calculateNormalMatrix(const glm::mat4 &model_matrix);

// Collision detection
// This is required for the physics engine to calculate sliding against
// walls/floors
bool checkSphereTriangleCollision(const glm::vec3 &sphere_center,
                                  float sphere_radius,
                                  const Collision::Triangle &triangle,
                                  glm::vec3 &collision_normal,
                                  float &penetration_depth);

} // namespace MathUtils

#endif
