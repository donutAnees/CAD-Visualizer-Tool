#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <limits>
#include <algorithm>
#include "mesh.h"
#include "ray.h"

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
*/

#define BVH_MAX_DEPTH 10

class BVH {
private:
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

	void traverseBVH(BVHNode* node, const Ray& ray, std::vector<Face*>& hitFaces) {

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

	void build(const std::vector<Mesh>& meshes) {
		triangles.clear();
		for (const auto& mesh : meshes) {
			for (const auto& face : mesh.faces) {
				triangles.push_back(const_cast<Face*>(&face));
			}
		}
		if (root) delete root;
		root = new BVHNode(0, static_cast<unsigned int>(triangles.size()));
		buildBVH(root, BVH_MAX_DEPTH);
	}

	std::vector<Face*> intersect(const Ray& ray);
};