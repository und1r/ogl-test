#include "GeometryUtils.h"
#include <cmath>
#include <limits> // For std::numeric_limits

const float PI = 3.14159265359f;

// Helper to calculate a single point on a sphere
Vertex getSphereVertex(float radius, float stack_angle, float sector_angle) {
    float xy = radius * cosf(stack_angle);
    float z = radius * sinf(stack_angle);
    float x = xy * cosf(sector_angle);
    float y = xy * sinf(sector_angle);

    Vertex v;
    v.position = glm::vec3(x, z, y);
    v.normal = glm::vec3(x / radius, z / radius, y / radius);
    // Simple UV mapping
    v.texCoords = glm::vec2(sector_angle / (2 * PI), stack_angle / PI + 0.5f);
    return v;
}

std::vector<Vertex> MathUtils::generateSphereVertices(float radius,
                                                      int stack_count,
                                                      int sector_count) {
    std::vector<Vertex> vertices_out;

    float sector_step = 2 * PI / sector_count;
    float stack_step = PI / stack_count;
    float sector_angle, stack_angle;

    for (int i = 0; i < stack_count; ++i) {
        stack_angle = PI / 2 - i * stack_step;
        float next_stack_angle = PI / 2 - (i + 1) * stack_step;

        for (int j = 0; j <= sector_count; ++j) {
            sector_angle = j * sector_step;

            // 4 corners of a "quad" on the sphere surface
            Vertex v1 = getSphereVertex(radius, stack_angle, sector_angle);
            Vertex v2 = getSphereVertex(radius, next_stack_angle, sector_angle);
            Vertex v3 = getSphereVertex(radius, stack_angle,
                                        sector_angle + sector_step);
            Vertex v4 = getSphereVertex(radius, next_stack_angle,
                                        sector_angle + sector_step);

            // Push 2 triangles per quad
            // Triangle 1
            vertices_out.push_back(v1);
            vertices_out.push_back(v2);
            vertices_out.push_back(v3);

            // Triangle 2
            vertices_out.push_back(v3);
            vertices_out.push_back(v2);
            vertices_out.push_back(v4);
        }
    }
    return vertices_out;
}

std::vector<Vertex> MathUtils::generatePlaneVertices(float size) {
    std::vector<Vertex> vertices_out;
    float half_size = size / 2.0f;

    // Define 6 vertices for a quad (2 triangles)
    // Triangle 1
    vertices_out.push_back(
        {glm::vec3(-half_size, 0.0f, -half_size), glm::vec3(0.0f, 1.0f, 0.0f)});
    vertices_out.push_back(
        {glm::vec3(-half_size, 0.0f, half_size), glm::vec3(0.0f, 1.0f, 0.0f)});
    vertices_out.push_back(
        {glm::vec3(half_size, 0.0f, half_size), glm::vec3(0.0f, 1.0f, 0.0f)});

    // Triangle 2
    vertices_out.push_back(
        {glm::vec3(-half_size, 0.0f, -half_size), glm::vec3(0.0f, 1.0f, 0.0f)});
    vertices_out.push_back(
        {glm::vec3(half_size, 0.0f, half_size), glm::vec3(0.0f, 1.0f, 0.0f)});
    vertices_out.push_back(
        {glm::vec3(half_size, 0.0f, -half_size), glm::vec3(0.0f, 1.0f, 0.0f)});

    return vertices_out;
}

glm::mat3 MathUtils::calculateNormalMatrix(const glm::mat4 &model_matrix) {
    // Normal matrix is the transpose of the inverse of the upper-left 3x3 model
    // matrix
    return glm::transpose(glm::inverse(glm::mat3(model_matrix)));
}


// Helper function to find the closest point on a segment to a point
glm::vec3 closestPointOnSegment(glm::vec3 p, glm::vec3 a, glm::vec3 b) {
    glm::vec3 ab = b - a;
    float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
    t = glm::clamp(t, 0.0f, 1.0f);
    return a + t * ab;
}

// Helper function to find the closest point on a triangle to a point
glm::vec3 closestPointOnTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 bc = c - b;

    // Check if P in vertex region outside A
    float d1 = glm::dot(ab, p - a);
    float d2 = glm::dot(ac, p - a);
    if (d1 <= 0.0f && d2 <= 0.0f) return a;

    // Check if P in vertex region outside B
    float d3 = glm::dot(ab, p - b);
    float d4 = glm::dot(ac, p - b);
    if (d3 >= 0.0f && d4 <= d3) return b;

    // Check if P in edge region of AB
    float vc = d1*d4 - d3*d2;
    if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
        return closestPointOnSegment(p, a, b);
    }

    // Check if P in vertex region outside C
    float d5 = glm::dot(ab, p - c);
    float d6 = glm::dot(ac, p - c);
    if (d6 >= 0.0f && d5 <= d6) return c;

    // Check if P in edge region of AC
    float vb = d5*d2 - d1*d6;
    if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
        return closestPointOnSegment(p, a, c);
    }

    // Check if P in edge region of BC
    float va = d3*d6 - d5*d4;
    if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
        return closestPointOnSegment(p, b, c);
    }

    // P inside face region. Compute Q as projection of P onto plane of triangle
    glm::vec3 n = glm::normalize(glm::cross(ab, ac)); // Triangle normal
    float dist = glm::dot(p - a, n);
    glm::vec3 q = p - dist * n;
    return q;
}


bool MathUtils::checkSphereTriangleCollision(const glm::vec3& sphere_center, float sphere_radius, const Collision::Triangle& triangle, glm::vec3& collision_normal, float& penetration_depth) {
    glm::vec3 closest_point = closestPointOnTriangle(sphere_center, triangle.v0, triangle.v1, triangle.v2);

    glm::vec3 vec_from_closest_point = sphere_center - closest_point;
    float distance_sq = glm::dot(vec_from_closest_point, vec_from_closest_point);
    float sphere_radius_sq = sphere_radius * sphere_radius;

    if (distance_sq < sphere_radius_sq) {
        // Collision occurred
        float distance = glm::sqrt(distance_sq);
        penetration_depth = sphere_radius - distance;

        if (distance == 0.0f) { // Sphere center is exactly on the triangle
            // In this specific case, use the triangle's normal for collision response
            collision_normal = glm::normalize(glm::cross(triangle.v1 - triangle.v0, triangle.v2 - triangle.v0));
        } else {
            collision_normal = glm::normalize(vec_from_closest_point);
        }
        return true;
    }
    return false;
}
