#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <limits>
#include <algorithm>
#include <stack>
#include <queue>
#include "mesh.h"
#include "ray.h"

// Define optimization macros to choose between memory or performance optimization
#define SPACIAL_OPT_MEMORY 0
#define SPACIAL_OPT_PERFORMANCE 1

// Default to performance optimization if not specified
#ifndef SPACIAL_OPT_MODE
#define SPACIAL_OPT_MODE SPACIAL_OPT_PERFORMANCE
#endif

/*
* Bounding Volume Hierarchy (BVH) is a tree structure on a set of geometric objects.
* Bounding Volumes enclose the objects tightly and if a ray does not intersect with the BV then we can safely ignore the objects inside that BV.
* 
* Saves computation time by rejecting BV that do not intersect with the ray.
* 
* Axis Aligned Bounding Boxes (AABB) are the most common type of BV.
* Defined by two points: minimum and maximum coordinates in 3D space.
* Merging two AABBs is done by taking the minimum and maximum coordinates of the two boxes.
* 
* In BVH, each node contains a bounding volume that encloses all the child nodes.
* The root node contains the bounding volume that encloses all the objects in the scene.
* Every node can be a leaf node or an internal node.
* Leaf node contains the triangles itself or objects.
* Internal node contains the bounding volume that encloses all the child nodes.
* 
* Considering the basis axis (1,0,0), (0,1,0), (0,0,1) for building the BVH.
* 
* Ways for splitting the BVH:
* 1. Spatial Median: Split the space into two halves along one of the axes, assign triangles based on their centroids on which space.
* 2. Object Median: Sort triangles with respect to centroid and assign them to left or right child based on the median.
* 3. Surface Area Heuristic (SAH): Choose the split that minimizes the expected cost of traversing the BVH.
* 
* SAH:
* Probability of hitting a triangle is proportional to its surface area.
* Therefore we should choose the split that minimizes the number of triangles in the child nodes.
* 
* To split the nodes, find a hyperplane b that minimizes the cost function:
* C(b) = P(left) * N(left) + P(right) * N(right)
* 
* Where P is the surface area of the bounding volume and N is the number of triangles in the child nodes.
* 
* KD-Tree:
* A KD-Tree is a space-partitioning data structure for organizing points in a k-dimensional space.
* Each non-leaf node implicitly generates a splitting hyperplane that divides the space into two parts.
* Points to the left of the hyperplane are represented by the left subtree and points to the right by the right subtree.
* 
* Splitting in a KD-Tree alternates between dimensions, cycling through all of the dimensions.
* For example, in 3D space, the splitting dimension cycles through x, y, and z.
* 
* Differences between BVH and KD-Tree:
* - BVH subdivides objects, while KD-Tree subdivides space
* - In BVH, an object is only in one node, but in KD-Tree, objects can be in multiple nodes
* - BVH is more memory-efficient but can have overlapping bounding boxes
* - KD-Tree is typically faster for ray tracing but requires more memory and preprocessing time
*/

#define BVH_MAX_DEPTH 10
#define KDT_MAX_DEPTH 16

// Base class for spatial acceleration structures
class SpatialAccelerator {
public:
    virtual ~SpatialAccelerator() {}
    virtual void build(const std::vector<Mesh>& meshes) = 0;
    virtual void traverse(void* node, const Ray& ray, std::vector<Face*>& hitFaces) = 0;
    virtual void drawDebug() const = 0;
};

// BVH implementation (more memory efficient)
class BVH : public SpatialAccelerator {
public:
    struct BVHNode {
        AABB boundingBox; // Bounding box of the node
        unsigned int startIndex; // Start index of the triangles in the vector
        unsigned int endIndex; // End index of the triangles in the vector
        BVHNode* left; // Left child
        BVHNode* right; // Right child
        BVHNode(unsigned int start, unsigned int end)
            : boundingBox(), startIndex(start), endIndex(end), left(nullptr), right(nullptr) {
        }
        ~BVHNode() {
            delete left;
            delete right;
        }
    };

private:
    BVHNode* root; // Root node of the BVH
    std::vector<Face*> triangles;// Pointer to the vector of faces (triangles), all childs will use the same vector

    void buildBVH(BVHNode* node, int depth = BVH_MAX_DEPTH) {
        unsigned int count = node->endIndex - node->startIndex;
        int splitIndex = splitNode(node, node->startIndex, node->endIndex, depth);
        if (splitIndex < 0 || depth <= 0 || count <= 1) {
            // Leaf node: compute bounding box for this range
            node->boundingBox = computeBoundingBox(node->startIndex, node->endIndex);
            return;
        }
        // Create left and right child nodes with new index ranges
        node->left = new BVHNode(node->startIndex, splitIndex);
        node->right = new BVHNode(splitIndex, node->endIndex);
        buildBVH(node->left, depth - 1);
        buildBVH(node->right, depth - 1);
        // Merge bounding boxes
        node->boundingBox = node->left->boundingBox;
        node->boundingBox.merge(node->right->boundingBox);
    }

    // Returns the index of the axis to split on
    int splitNode(BVHNode* node, unsigned int start, unsigned int end, int depth = BVH_MAX_DEPTH) {
        unsigned int N = end - start;
        if (N <= 1) return -1;

        int bestAxis = -1;
        int bestSplit = -1;
        float bestCost = std::numeric_limits<float>::max();

        // Try all 3 axes (0: x, 1: y, 2: z)
        for (int axis = 0; axis < 3; ++axis) {
            // Sort triangles in-place by centroid along this axis
            std::sort(triangles.begin() + start, triangles.begin() + end,
                [axis](const Face* a, const Face* b) {
                    return a->centroid[axis] < b->centroid[axis];
                });

            // Precompute left and right surface areas using the existing function
            std::vector<float> leftSurfaceArea, rightSurfaceArea;
            computeSurfaceArea(start, end, leftSurfaceArea, rightSurfaceArea);

            // Evaluate SAH cost for all possible splits
            for (unsigned int i = 1; i < N; ++i) {
                float cost = leftSurfaceArea[i] * i + rightSurfaceArea[i] * (N - i);
                if (cost < bestCost) {
                    bestCost = cost;
                    bestAxis = axis;
                    bestSplit = i;
                }
            }
        }

        if (bestSplit <= 0 || bestSplit >= (int)N) return -1;

        // Resort triangles along the best axis for the actual split
        std::sort(triangles.begin() + start, triangles.begin() + end,
            [bestAxis](const Face* a, const Face* b) {
                return a->centroid[bestAxis] < b->centroid[bestAxis];
            });

        return start + bestSplit;
    }

    void computeSurfaceArea(unsigned int start, unsigned int end, std::vector<float>& leftSurfaceArea, std::vector<float>& rightSurfaceArea) {
        int size = end - start;
        leftSurfaceArea.resize(size);
        rightSurfaceArea.resize(size);
        leftSurfaceArea[0] = 0.0f;
        rightSurfaceArea[size - 1] = 0.0f;
        AABB leftBox, rightBox;
        for (int i = 1; i < size; ++i) {
            leftBox.merge(triangles[start + i - 1]->boundingBox);
            leftSurfaceArea[i] = leftBox.getSurfaceArea();
        }
        for (int i = size - 2; i >= 0; --i) {
            rightBox.merge(triangles[start + i + 1]->boundingBox);
            rightSurfaceArea[i] = rightBox.getSurfaceArea();
        }
    }

    AABB computeBoundingBox(unsigned int start, unsigned int end) {
        AABB box;
        if (start >= end) return box;
        box.min = glm::vec3(FLT_MAX);
        box.max = glm::vec3(-FLT_MAX);
        for (unsigned int i = start; i < end; ++i) {
            box.merge(triangles[i]->boundingBox);
        }
        return box;
    }
    
public:
    BVH() : root(nullptr) {}
    ~BVH() {
        delete root;
    }

    BVHNode* getRoot() const {
        return root;
    }

    void build(const std::vector<Mesh>& meshes) override {
        triangles.clear();
        for (const Mesh& mesh : meshes) {
            for (const Face& face : mesh.faces) {
                triangles.push_back(const_cast<Face*>(&face));
            }
        }
        if (triangles.empty()) {
            root = nullptr;
            return;
        }
        if (root) delete root;
        root = new BVHNode(0, static_cast<unsigned int>(triangles.size()));
        buildBVH(root, BVH_MAX_DEPTH);
    }

    void traverse(void* node_ptr, const Ray& ray, std::vector<Face*>& hitFaces) override {
        BVHNode* node = static_cast<BVHNode*>(node_ptr);
        if (!node) return;
        
        std::stack<BVHNode*> stack;
        stack.push(node);
        while (!stack.empty()) {
            BVHNode* current = stack.top();
            stack.pop();
            if (!current || !current->boundingBox.isIntersectingRay(ray.origin, ray.direction, ray.tMin, ray.tMax)) continue;
            // If it's a leaf node, check for intersections with triangles
            if (current->left == nullptr && current->right == nullptr) {
                for (unsigned int i = current->startIndex; i < current->endIndex; ++i) {
                    if (triangles[i]->isIntersectingRay(ray.origin, ray.direction, ray.tMin, ray.tMax)) {
                        hitFaces.push_back(triangles[i]);
                    }
                }
                continue;
            }
            // Push children onto the stack
            if (current->left) stack.push(current->left);
            if (current->right) stack.push(current->right);
        }
    }

    void drawDebug() const override {
        drawBVHNode(root);
    }

    void drawBVHNode(const BVHNode* node) const {
        if (!node) return;

        // Draw the bounding box of the current node
        const glm::vec3& min = node->boundingBox.min;
        const glm::vec3& max = node->boundingBox.max;

        glColor3f(0.0f, 1.0f, 0.0f); // Green color for bounding boxes
        glBegin(GL_LINES);

        // Bottom face
        glVertex3f(min.x, min.y, min.z); glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, min.y, min.z); glVertex3f(max.x, min.y, max.z);
        glVertex3f(max.x, min.y, max.z); glVertex3f(min.x, min.y, max.z);
        glVertex3f(min.x, min.y, max.z); glVertex3f(min.x, min.y, min.z);

        // Top face
        glVertex3f(min.x, max.y, min.z); glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, max.y, min.z); glVertex3f(max.x, max.y, max.z);
        glVertex3f(max.x, max.y, max.z); glVertex3f(min.x, max.y, max.z);
        glVertex3f(min.x, max.y, max.z); glVertex3f(min.x, max.y, min.z);

        // Vertical edges
        glVertex3f(min.x, min.y, min.z); glVertex3f(min.x, max.y, min.z);
        glVertex3f(max.x, min.y, min.z); glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, min.y, max.z); glVertex3f(max.x, max.y, max.z);
        glVertex3f(min.x, min.y, max.z); glVertex3f(min.x, max.y, max.z);

        glEnd();

        // Recursively draw child nodes if they exist
        drawBVHNode(node->left);
        drawBVHNode(node->right);
    }
};

// KD-Tree implementation (better performance)
class KDTree : public SpatialAccelerator {
public:
    struct KDTreeNode {
        AABB boundingBox; // Bounding box of the node
        float splitPosition; // Position of the splitting plane
        int splitAxis; // Axis of the splitting plane (0: x, 1: y, 2: z)
        std::vector<Face*> faces; // Faces in this node (only for leaf nodes)
        KDTreeNode* left; // Left child
        KDTreeNode* right; // Right child
        bool isLeaf; // Is this a leaf node?

        KDTreeNode() : splitPosition(0.0f), splitAxis(-1), left(nullptr), right(nullptr), isLeaf(false) {}
        
        ~KDTreeNode() {
            delete left;
            delete right;
        }
    };

private:
    KDTreeNode* root; // Root node of the KD-Tree
    std::vector<Face*> triangles; // All faces in the scene

    void buildKDTree(KDTreeNode* node, const std::vector<Face*>& faces, int depth) {
        // Always initialize the bounding box, even for empty face lists
        if (faces.empty()) {
            node->boundingBox.min = glm::vec3(0.0f);
            node->boundingBox.max = glm::vec3(0.0f);
            node->isLeaf = true;
            return;
        }

        // Compute bounding box for all nodes
        node->boundingBox = computeBoundingBox(faces);

        // If we've reached max depth or there are few enough faces, make it a leaf
        if (depth >= KDT_MAX_DEPTH || faces.size() <= 4) {
            node->isLeaf = true;
            node->faces = faces;
            return;
        }

        // Choose the axis to split on (cycle through x, y, z)
        int axis = depth % 3;
        node->splitAxis = axis;

        // Find the median position along this axis
        std::vector<float> positions;
        for (const Face* face : faces) {
            positions.push_back(face->centroid[axis]);
        }
        
        if (positions.empty()) { // Extra safety check
            node->isLeaf = true;
            return;
        }

        std::sort(positions.begin(), positions.end());
        float splitPos = positions[positions.size() / 2];
        node->splitPosition = splitPos;

        // Split the faces
        std::vector<Face*> leftFaces, rightFaces;
        for (const Face* face : faces) {
            // Check if the face spans the splitting plane
            if (face->boundingBox.min[axis] <= splitPos) {
                leftFaces.push_back(const_cast<Face*>(face));
            }
            
            if (face->boundingBox.max[axis] >= splitPos) {
                rightFaces.push_back(const_cast<Face*>(face));
            }
        }

        // If splitting didn't reduce faces, make it a leaf
        if ((leftFaces.size() == faces.size() && rightFaces.size() == faces.size()) || 
            leftFaces.empty() || rightFaces.empty()) {
            node->isLeaf = true;
            node->faces = faces;
            return;
        }

        // Create child nodes
        node->left = new KDTreeNode();
        node->right = new KDTreeNode();
        
        // Recursively build the tree
        buildKDTree(node->left, leftFaces, depth + 1);
        buildKDTree(node->right, rightFaces, depth + 1);
    }

    AABB computeBoundingBox(const std::vector<Face*>& faces) {
        AABB box;
        if (faces.empty()) return box;
        
        box.min = glm::vec3(FLT_MAX);
        box.max = glm::vec3(-FLT_MAX);
        
        for (const Face* face : faces) {
            box.merge(face->boundingBox);
        }
        
        return box;
    }

public:
    KDTree() : root(nullptr) {}
    ~KDTree() {
        delete root;
    }

    KDTreeNode* getRoot() const {
        return root;
    }

    void build(const std::vector<Mesh>& meshes) override {
        triangles.clear();
        for (const Mesh& mesh : meshes) {
            for (const Face& face : mesh.faces) {
                triangles.push_back(const_cast<Face*>(&face));
            }
        }
        
        if (triangles.empty()) {
            root = nullptr;
            return;
        }
        
        if (root) delete root;
        root = new KDTreeNode();
        buildKDTree(root, triangles, 0);
    }

    void traverse(void* node_ptr, const Ray& ray, std::vector<Face*>& hitFaces) override {
        KDTreeNode* node = static_cast<KDTreeNode*>(node_ptr);
        if (!node) return;
        
        // Non-recursive traversal using a stack
        struct StackEntry {
            KDTreeNode* node;
            float tMin, tMax;
            StackEntry(KDTreeNode* n, float min, float max) : node(n), tMin(min), tMax(max) {}
        };
        
        std::stack<StackEntry> stack;
        stack.push(StackEntry(node, ray.tMin, ray.tMax));
        
        while (!stack.empty()) {
            StackEntry entry = stack.top();
            stack.pop();
            
            KDTreeNode* current = entry.node;
            float tMin = entry.tMin;
            float tMax = entry.tMax;
            
            // Skip if node is null or has invalid bounding box
            if (!current || glm::any(glm::isnan(current->boundingBox.min)) ||
                glm::any(glm::isnan(current->boundingBox.max))) {
                continue;
            }
            
            // Skip if the ray doesn't intersect the node's bounding box
            if (!current->boundingBox.isIntersectingRay(ray.origin, ray.direction, tMin, tMax)) {
                continue;
            }
            
            // If it's a leaf node, check for intersections with triangles
            if (current->isLeaf) {
                for (Face* face : current->faces) {
                    if (face && face->isIntersectingRay(ray.origin, ray.direction, ray.tMin, ray.tMax)) {
                        hitFaces.push_back(face);
                    }
                }
                continue;
            }
            
            // Skip if split axis is invalid
            if (current->splitAxis < 0 || current->splitAxis > 2) {
                continue;
            }
            
            // Calculate intersection with the splitting plane
            int axis = current->splitAxis;
            float splitPos = current->splitPosition;
            float tSplit = (splitPos - ray.origin[axis]) / ray.direction[axis];
            
            // Determine which child to traverse first based on ray direction
            KDTreeNode* firstChild = nullptr;
            KDTreeNode* secondChild = nullptr;
            
            if (ray.origin[axis] < splitPos || (ray.origin[axis] == splitPos && ray.direction[axis] <= 0)) {
                firstChild = current->left;
                secondChild = current->right;
            } else {
                firstChild = current->right;
                secondChild = current->left;
            }
            
            // Handle parallel rays
            if (std::abs(ray.direction[axis]) < std::numeric_limits<float>::epsilon()) {
                if (firstChild) stack.push(StackEntry(firstChild, tMin, tMax));
            }
            // Handle general case
            else if (tSplit >= tMax || tSplit <= 0) {
                if (firstChild) stack.push(StackEntry(firstChild, tMin, tMax));
            }
            else if (tSplit <= tMin) {
                if (secondChild) stack.push(StackEntry(secondChild, tMin, tMax));
            }
            else {
                // Ray passes through the splitting plane, visit both children
                if (secondChild) stack.push(StackEntry(secondChild, tSplit, tMax));
                if (firstChild) stack.push(StackEntry(firstChild, tMin, tSplit));
            }
        }
    }

    void drawDebug() const override {
        drawKDTreeNode(root);
    }

    void drawKDTreeNode(const KDTreeNode* node) const {
        if (!node) return;
        
        // Safety check for invalid or uninitialized bounding boxes
        if (glm::any(glm::isnan(node->boundingBox.min)) ||
            glm::any(glm::isnan(node->boundingBox.max)) ||
            glm::any(glm::isinf(node->boundingBox.min)) ||
            glm::any(glm::isinf(node->boundingBox.max))) {
            return;
        }
        
        // Draw the bounding box
        const glm::vec3& min = node->boundingBox.min;
        const glm::vec3& max = node->boundingBox.max;
        
        glColor3f(0.0f, 0.0f, 1.0f); // Blue color for KD-Tree
        glBegin(GL_LINES);
        
        // Bottom face
        glVertex3f(min.x, min.y, min.z); glVertex3f(max.x, min.y, min.z);
        glVertex3f(max.x, min.y, min.z); glVertex3f(max.x, min.y, max.z);
        glVertex3f(max.x, min.y, max.z); glVertex3f(min.x, min.y, max.z);
        glVertex3f(min.x, min.y, max.z); glVertex3f(min.x, min.y, min.z);
        
        // Top face
        glVertex3f(min.x, max.y, min.z); glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, max.y, min.z); glVertex3f(max.x, max.y, max.z);
        glVertex3f(max.x, max.y, max.z); glVertex3f(min.x, max.y, max.z);
        glVertex3f(min.x, max.y, max.z); glVertex3f(min.x, max.y, min.z);

        // Vertical edges
        glVertex3f(min.x, min.y, min.z); glVertex3f(min.x, max.y, min.z);
        glVertex3f(max.x, min.y, min.z); glVertex3f(max.x, max.y, min.z);
        glVertex3f(max.x, min.y, max.z); glVertex3f(max.x, max.y, max.z);
        glVertex3f(min.x, min.y, max.z); glVertex3f(min.x, max.y, max.z);

        glEnd();
        
        // Draw the splitting plane if not a leaf
        if (!node->isLeaf && node->splitAxis >= 0 && node->splitAxis <= 2) {
            glColor3f(1.0f, 0.0f, 0.0f); // Red color for splitting planes
            glBegin(GL_QUADS);
            
            int axis = node->splitAxis;
            float pos = node->splitPosition;
            
            if (axis == 0) { // X-axis
                glVertex3f(pos, min.y, min.z);
                glVertex3f(pos, max.y, min.z);
                glVertex3f(pos, max.y, max.z);
                glVertex3f(pos, min.y, max.z);
            } else if (axis == 1) { // Y-axis
                glVertex3f(min.x, pos, min.z);
                glVertex3f(max.x, pos, min.z);
                glVertex3f(max.x, pos, max.z);
                glVertex3f(min.x, pos, max.z);
            } else if (axis == 2) { // Z-axis
                glVertex3f(min.x, min.y, pos);
                glVertex3f(max.x, min.y, pos);
                glVertex3f(max.x, max.y, pos);
                glVertex3f(min.x, max.y, pos);
            }
            
            glEnd();
        }
        
        // Recursively draw child nodes with safety checks
        if (node->left) drawKDTreeNode(node->left);
        if (node->right) drawKDTreeNode(node->right);
    }
};

// Factory class to create the appropriate accelerator based on optimization mode
class SpatialAcceleratorFactory {
public:
    static SpatialAccelerator* createAccelerator() {
#if SPACIAL_OPT_MODE == SPACIAL_OPT_MEMORY
        return new BVH();
#else
        return new KDTree();
#endif
    }
};