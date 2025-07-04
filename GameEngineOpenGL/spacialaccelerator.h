#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "mesh.h"

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
	int splitNode(BVHNode* node, unsigned int start, unsigned int end, int depth = BVH_MAX_DEPTH){
	
	}

	void computeSurfaceArea(unsigned int start, unsigned int end, std::vector<float>& leftSurfaceArea, std::vector<float>& rightSurfaceArea) {
		int size = end - start;
		leftSurfaceArea.resize(size);
		rightSurfaceArea.resize(size);
		leftSurfaceArea[0] = 0.0f;
		rightSurfaceArea[size - 1] = 0.0f;
		AABB leftBox, rightBox;
		for (int i = 1; i < size; ++i) {
			leftBox.merge(allFaces[start + i - 1]->boundingBox);
			leftSurfaceArea[i] = leftBox.getSurfaceArea();
		}
		for (int i = size - 2; i >= 0; --i) {
			rightBox.merge(allFaces[start + i + 1]->boundingBox);
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
		allFaces.clear();
		for (const auto& mesh : meshes) {
			for (const auto& face : mesh.faces) {
				allFaces.push_back(const_cast<Face*>(&face));
			}
		}
		if (root) delete root;
		root = new BVHNode(0, static_cast<unsigned int>(allFaces.size()));
		buildBVH(root, BVH_MAX_DEPTH);
	}

	std::vector<Face*> intersect(const Ray& ray);
};