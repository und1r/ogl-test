#include "Octree.h"
#include <algorithm> // For std::min/max
#include <iostream>

namespace Collision {

// --- AABB Implementations ---
bool AABB::contains(const glm::vec3& point) const {
    return (point.x >= min.x && point.x <= max.x) &&
           (point.y >= min.y && point.y <= max.y) &&
           (point.z >= min.z && point.z <= max.z);
}

bool AABB::intersects(const AABB& other) const {
    return (min.x <= other.max.x && max.x >= other.min.x) &&
           (min.y <= other.max.y && max.y >= other.min.y) &&
           (min.z <= other.max.z && max.z >= other.min.z);
}

// Helper to calculate the AABB of a triangle
AABB getTriangleAABB(const Triangle& triangle) {
    return {
        glm::min(glm::min(triangle.v0, triangle.v1), triangle.v2),
        glm::max(glm::max(triangle.v0, triangle.v1), triangle.v2)
    };
}


// --- OctreeNode Free Functions ---
void initOctreeNode(OctreeNode* node, const AABB& bounds) {
    node->bounds = bounds;
    for (int i = 0; i < 8; ++i) {
        node->children[i] = nullptr;
    }
    node->triangles.clear();
    node->is_leaf = true; // Initially all nodes are leaves
}

// Forward declarations for mutual recursion
void insertTriangleIntoOctree(OctreeNode* node, const Triangle& triangle, int current_depth, int max_depth, int triangles_per_node);

// Helper to subdivide a node
void subdivideNode(OctreeNode* node, int current_depth, int max_depth, int triangles_per_node) {
    glm::vec3 center = (node->bounds.min + node->bounds.max) * 0.5f;

    node->children[0] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[0].get(), AABB{node->bounds.min, center});
    node->children[1] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[1].get(), AABB{glm::vec3(center.x, node->bounds.min.y, node->bounds.min.z), glm::vec3(node->bounds.max.x, center.y, center.z)});
    node->children[2] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[2].get(), AABB{glm::vec3(node->bounds.min.x, center.y, node->bounds.min.z), glm::vec3(center.x, node->bounds.max.y, center.z)});
    node->children[3] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[3].get(), AABB{glm::vec3(center.x, center.y, node->bounds.min.z), glm::vec3(node->bounds.max.x, node->bounds.max.y, center.z)});
    node->children[4] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[4].get(), AABB{glm::vec3(node->bounds.min.x, node->bounds.min.y, center.z), glm::vec3(center.x, center.y, node->bounds.max.z)});
    node->children[5] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[5].get(), AABB{glm::vec3(center.x, node->bounds.min.y, center.z), glm::vec3(node->bounds.max.x, center.y, node->bounds.max.z)});
    node->children[6] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[6].get(), AABB{glm::vec3(node->bounds.min.x, center.y, center.z), glm::vec3(center.x, node->bounds.max.y, node->bounds.max.z)});
    node->children[7] = std::make_unique<OctreeNode>(); initOctreeNode(node->children[7].get(), AABB{center, node->bounds.max});

    node->is_leaf = false; // Node is no longer a leaf

    // Redistribute triangles from parent to children
    std::vector<Triangle> triangles_to_redistribute;
    triangles_to_redistribute.swap(node->triangles); // Efficiently clear and move

    for (const auto& tri : triangles_to_redistribute) {
        int octant_index = getContainingOctant(node, tri); // Which child does this triangle fit into?
        if (octant_index != -1 && node->children[octant_index]) {
            insertTriangleIntoOctree(node->children[octant_index].get(), tri, current_depth + 1, max_depth, triangles_per_node);
        } else {
            // If it doesn't fit into one child, add it back to the current (now subdivided) node.
            node->triangles.push_back(tri);
        }
    }
}


// --- Octree Free Functions ---
Octree createOctree(const AABB& bounds, int max_depth, int triangles_per_node) {
    Octree octree;
    octree.root = std::make_unique<OctreeNode>();
    initOctreeNode(octree.root.get(), bounds);
    octree.max_depth = max_depth;
    octree.triangles_per_node = triangles_per_node;
    return octree;
}

bool isTriangleWhollyContainedInAABB(const AABB& aabb, const Triangle& triangle) {
    return aabb.contains(triangle.v0) && aabb.contains(triangle.v1) && aabb.contains(triangle.v2);
}

int getContainingOctant(const OctreeNode* node, const Triangle& triangle) {
    if (node->is_leaf) return -1; // Not subdivided yet

    for (int i = 0; i < 8; ++i) {
        if (node->children[i] && isTriangleWhollyContainedInAABB(node->children[i]->bounds, triangle)) {
            return i; // Found an octant that wholly contains the triangle
        }
    }
    return -1; // Triangle spans multiple octants or is outside
}

void insertTriangleIntoOctree(OctreeNode* node, const Triangle& triangle, int current_depth, int max_depth, int triangles_per_node) {
    AABB triangle_aabb = getTriangleAABB(triangle);

    // If triangle does not intersect node bounds, return
    if (!node->bounds.intersects(triangle_aabb)) {
        return;
    }

    // Condition to subdivide or add to current node
    // If max depth reached, OR node is a leaf AND has fewer than `triangles_per_node` triangles,
    // then add the triangle to this node.
    if (current_depth >= max_depth || (node->is_leaf && node->triangles.size() < triangles_per_node)) {
        node->triangles.push_back(triangle);
        return;
    }

    // If we are here, the node either needs to subdivide or is already subdivided,
    // and the triangle needs to be placed into a child (or stay in this node if it spans).

    // If it's a leaf node but exceeds triangle limit and can subdivide:
    if (node->is_leaf) {
        subdivideNode(node, current_depth, max_depth, triangles_per_node);
        // After subdivision, existing triangles are redistributed.
        // Now, this `triangle` also needs to be handled by the logic below.
    }

    // Try to insert into a child octant
    int octant_index = getContainingOctant(node, triangle);
    if (octant_index != -1 && node->children[octant_index]) {
        // It fits wholly into a child, insert it there
        insertTriangleIntoOctree(node->children[octant_index].get(), triangle, current_depth + 1, max_depth, triangles_per_node);
    } else {
        // Triangle spans multiple octants or doesn't fit wholly into one child.
        // Add it to the current node.
        node->triangles.push_back(triangle);
    }
}

void getTrianglesFromOctree(const OctreeNode* node, const AABB& query_bounds, std::vector<Triangle>& out_triangles) {
    if (!node || !node->bounds.intersects(query_bounds)) {
        return;
    }

    // Add triangles directly stored in this node
    for (const auto& triangle : node->triangles) {
        out_triangles.push_back(triangle);
    }

    // Recursively check children if subdivided
    if (!node->is_leaf) {
        for (int i = 0; i < 8; ++i) {
            getTrianglesFromOctree(node->children[i].get(), query_bounds, out_triangles);
        }
    }
}

} // namespace Collision