#ifndef OCTREE_H
#define OCTREE_H

#include <glm/glm.hpp>
#include <vector>
#include <memory> // For std::unique_ptr

namespace Collision {

struct AABB {
    glm::vec3 min;
    glm::vec3 max;

    bool contains(const glm::vec3& point) const;
    bool intersects(const AABB& other) const;
};

struct Triangle {
    glm::vec3 v0, v1, v2;
};

// Forward declaration of OctreeNode for use in Octree struct
struct OctreeNode;

// Free functions for OctreeNode operations
void initOctreeNode(OctreeNode* node, const AABB& bounds);

struct OctreeNode {
    AABB bounds;
    std::unique_ptr<OctreeNode> children[8]; // Unique pointers for ownership
    std::vector<Triangle> triangles;
    bool is_leaf; // Flag to indicate if node has children
};

// Main Octree struct (data-only)
struct Octree {
    std::unique_ptr<OctreeNode> root;
    int max_depth; // Max subdivision depth
    int triangles_per_node; // Max triangles before subdivision
};

// Free functions for Octree operations
Octree createOctree(const AABB& bounds, int max_depth = 8, int triangles_per_node = 8);
void insertTriangleIntoOctree(OctreeNode* node, const Triangle& triangle, int current_depth, int max_depth, int triangles_per_node);
void getTrianglesFromOctree(const OctreeNode* node, const AABB& query_bounds, std::vector<Triangle>& out_triangles);

// Helper functions for triangle-octant relationship
bool isTriangleWhollyContainedInAABB(const AABB& aabb, const Triangle& triangle);
int getContainingOctant(const OctreeNode* node, const Triangle& triangle);

} // namespace Collision

#endif // OCTREE_H