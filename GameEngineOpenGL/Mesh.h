#pragma once

#include <GL/gl.h>
#include <GL/glu.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <iostream>
#include <glm/glm.hpp>
#include <windows.h>

// Axis-Aligned Bounding Box
class AABB {
public:
    glm::vec3 min; // Minimum coordinates
    glm::vec3 max; // Maximum coordinates
    
    // Default constructor - creates an invalid AABB
    AABB() : min(FLT_MAX), max(-FLT_MAX) {}
    
    // Construct from min/max points
    AABB(const glm::vec3& minPoint, const glm::vec3& maxPoint) : min(minPoint), max(maxPoint) {}
    
    // Merge with another AABB
    void merge(const AABB& other) {
        min = glm::min(min, other.min);
        max = glm::max(max, other.max);
    }
    
    // Get surface area for cost calculations
    float getSurfaceArea() const {
        glm::vec3 extent = max - min;
        return 2.0f * (extent.x * extent.y + extent.x * extent.z + extent.y * extent.z);
    }
    
    // Check if the AABB contains a point
    bool contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    // Get the center of the AABB
    glm::vec3 getCenter() const {
        return (min + max) * 0.5f;
    }
    
    // Get the size of the AABB
    glm::vec3 getSize() const {
        return max - min;
    }
    
    // Check if the AABB is valid
    bool isValid() const {
        return min.x <= max.x && min.y <= max.y && min.z <= max.z;
    }

    // Ray-AABB intersection test
    bool isIntersectingRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float tMin, float tMax) const {
        // Handle zero components in ray direction to avoid division by zero
        glm::vec3 invDir;
        invDir.x = rayDirection.x != 0.0f ? 1.0f / rayDirection.x : FLT_MAX;
        invDir.y = rayDirection.y != 0.0f ? 1.0f / rayDirection.y : FLT_MAX;
        invDir.z = rayDirection.z != 0.0f ? 1.0f / rayDirection.z : FLT_MAX;
        
        // Calculate t-values for each slab intersection
        float t1 = (min.x - rayOrigin.x) * invDir.x;
        float t2 = (max.x - rayOrigin.x) * invDir.x;
        float t3 = (min.y - rayOrigin.y) * invDir.y;
        float t4 = (max.y - rayOrigin.y) * invDir.y;
        float t5 = (min.z - rayOrigin.z) * invDir.z;
        float t6 = (max.z - rayOrigin.z) * invDir.z;
        
        // Find min/max t values for each axis
        float tMinX = std::min(t1, t2);
        float tMaxX = std::max(t1, t2);
        float tMinY = std::min(t3, t4);
        float tMaxY = std::max(t3, t4);
        float tMinZ = std::min(t5, t6);
        float tMaxZ = std::max(t5, t6);
        
        // Find the overall entry and exit points
        float tEnter = std::max({tMinX, tMinY, tMinZ, tMin});
        float tExit = std::min({tMaxX, tMaxY, tMaxZ, tMax});
        
        // Check if there's a valid intersection interval
        return tEnter <= tExit;
    }
};

// Face/Triangle class with AABB for spatial acceleration
class Face {
public:
    unsigned int v0, v1, v2;          // Indices of vertices in the mesh
    AABB boundingBox;                  // AABB for fast intersection rejection
    glm::vec3 centroid;                // Center point of the face (for BVH construction)
    std::vector<glm::vec3> vertices;   // Cached vertices for this face
    
    // Default constructor
    Face() : v0(0), v1(0), v2(0), centroid(0.0f) {}
    
    // Construct from vertex indices and source vertices
    Face(unsigned int v0, unsigned int v1, unsigned int v2, std::vector<GLfloat>* sourceVertices)
        : v0(v0), v1(v1), v2(v2)
    {
        // Check if indices are valid
        if (!sourceVertices || sourceVertices->size() < 3 * (std::max({ v0, v1, v2 }) + 1)) {
            throw std::out_of_range("Face vertex index out of bounds");
        }

        // Cache the vertex positions for this face
        vertices.reserve(3);
        vertices.push_back(glm::vec3(
            (*sourceVertices)[v0 * 3],
            (*sourceVertices)[v0 * 3 + 1],
            (*sourceVertices)[v0 * 3 + 2]));

        vertices.push_back(glm::vec3(
            (*sourceVertices)[v1 * 3],
            (*sourceVertices)[v1 * 3 + 1],
            (*sourceVertices)[v1 * 3 + 2]));

        vertices.push_back(glm::vec3(
            (*sourceVertices)[v2 * 3],
            (*sourceVertices)[v2 * 3 + 1],
            (*sourceVertices)[v2 * 3 + 2]));

        // Calculate AABB and centroid
        boundingBox.min = glm::min(glm::min(vertices[0], vertices[1]), vertices[2]);
        boundingBox.max = glm::max(glm::max(vertices[0], vertices[1]), vertices[2]);
        centroid = (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
    }
    
    // Get vertex index by position (0, 1, or 2)
    unsigned int getVertex(unsigned int index) const {
        switch (index) {
            case 0: return v0;
            case 1: return v1;
            case 2: return v2;
            default: throw std::out_of_range("Index must be 0, 1, or 2");
        }
    }
    
    // Ray-Triangle intersection test using Möller-Trumbore algorithm
    bool isIntersectingRay(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, float tMin, float tMax) const {
        // Early exit if we don't have exactly 3 vertices
        if (vertices.size() != 3) return false;
        
        // First do a quick AABB test to reject most non-intersecting rays
        if (!boundingBox.isIntersectingRay(rayOrigin, rayDirection, tMin, tMax)) {
            return false;
        }
        
        // Calculate edges of the triangle
        glm::vec3 edge1 = vertices[1] - vertices[0];
        glm::vec3 edge2 = vertices[2] - vertices[0];

        // Calculate determinant
        glm::vec3 h = glm::cross(rayDirection, edge2);
        float det = glm::dot(edge1, h);

        // If determinant is near zero, ray lies in plane of triangle or is parallel
        if (det > -FLT_EPSILON && det < FLT_EPSILON) return false;
        
        float invDet = 1.0f / det;

        // Calculate barycentric coordinate u
        glm::vec3 s = rayOrigin - vertices[0];
        float u = glm::dot(s, h) * invDet;
        if (u < 0.0f || u > 1.0f) return false; // Outside triangle bounds

        // Calculate barycentric coordinate v
        glm::vec3 q = glm::cross(s, edge1);
        float v = glm::dot(rayDirection, q) * invDet;
        if (v < 0.0f || u + v > 1.0f) return false; // Outside triangle bounds

        // Calculate intersection distance t
        float t = glm::dot(edge2, q) * invDet;

        // Check if intersection is within the valid range
        return t >= tMin && t <= tMax;
    }
};

// Mesh class representing a 3D object
class Mesh {
public:
    // *** Data members ***
    
    // Geometry data
    std::vector<GLfloat> vertices;             // Original vertices in 3D space (x0, y0, z0, x1, y1, z1...)
    std::vector<GLfloat> transformedVertices;  // Transformed vertices after applying model matrix
    std::vector<GLfloat> colors;               // Per-vertex colors
    std::vector<unsigned int> indices;         // Triangle indices (groups of 3)
    std::vector<Face> faces;                   // Triangle faces with cached data
    
    // Bounding volumes
    AABB aabb;                                 // Axis-aligned bounding box (AABB)
    glm::vec4 obbCorners[8];                   // Oriented bounding box (OBB) corners
    
    // Transform properties
    GLfloat centerX = 0.0f, centerY = 0.0f, centerZ = 0.0f;  // Center position in world space
    GLfloat sizeX = 0.0f, sizeY = 0.0f, sizeZ = 0.0f;        // Size of the AABB in world space
    GLfloat rotationX = 0.0f, rotationY = 0.0f, rotationZ = 0.0f;  // Rotation angles in degrees
    GLfloat scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;     // Scale factors
    glm::mat4 modelMatrix = glm::mat4(1.0f);   // Model matrix for transformations
    
    // Appearance properties
    GLfloat colorR = 1.0f, colorG = 1.0f, colorB = 1.0f;  // Base color
    GLfloat transparency = 0.0f;               // 0.0 = opaque, 1.0 = transparent
    GLfloat shininess = 0.0f;                  // Material shininess (0.0 - 1.0)
    int materialType = 0;                      // Material type (0 = default, 1 = plastic, etc.)
    bool isTransparent = false;                // Whether transparency is enabled
    
    // Identity properties
    std::string objectName = "Object";         // Name of the object
    std::string objectType = "Generic";        // Type of the object ("Cube", "Pyramid", etc.)
    
    // Display options
    bool isVisible = true;                     // Whether the mesh is visible
    bool wireframeMode = false;                // Whether to render in wireframe mode
    bool showBoundingBox = false;              // Whether to show the bounding box
    bool showVertices = false;                 // Whether to show vertex points
    bool isSelected = false;                   // Whether the mesh is selected
    int selectedFaceIndex = -1;                // Index of the selected face (-1 if none)

    // *** Constructors/Destructor ***
    
    // Default constructor
    Mesh() {}
    
    // Destructor - clean up any resources
    ~Mesh() {
        vertices.clear();
        transformedVertices.clear();
        indices.clear();
        colors.clear();
        faces.clear();
    }
    
    // *** Bounding Box Methods ***
    
    // Calculate object-space dimensions (before transformations)
    glm::vec3 getObjectSpaceDimensions() const {
        if (vertices.empty()) return glm::vec3(0.0f);
        
        float minX = vertices[0], maxX = vertices[0];
        float minY = vertices[1], maxY = vertices[1];
        float minZ = vertices[2], maxZ = vertices[2];

        for (size_t i = 3; i < vertices.size(); i += 3) {
            minX = std::min(minX, vertices[i]);
            maxX = std::max(maxX, vertices[i]);
            minY = std::min(minY, vertices[i + 1]);
            maxY = std::max(maxY, vertices[i + 1]);
            minZ = std::min(minZ, vertices[i + 2]);
            maxZ = std::max(maxZ, vertices[i + 2]);
        }
        
        return glm::vec3(maxX - minX, maxY - minY, maxZ - minZ);
    }
    
    // Calculate Axis-Aligned Bounding Box (AABB) from transformed vertices
    void calculateAABB() {
        if (transformedVertices.empty()) return;
        
        // Start with the first vertex
        glm::vec3 aabbMin(transformedVertices[0], transformedVertices[1], transformedVertices[2]);
        glm::vec3 aabbMax(transformedVertices[0], transformedVertices[1], transformedVertices[2]);

        // Find the minimum and maximum coordinates
        for (size_t i = 3; i < transformedVertices.size(); i += 3) {
            aabbMin.x = std::min(aabbMin.x, transformedVertices[i]);
            aabbMax.x = std::max(aabbMax.x, transformedVertices[i]);
            aabbMin.y = std::min(aabbMin.y, transformedVertices[i + 1]);
            aabbMax.y = std::max(aabbMax.y, transformedVertices[i + 1]);
            aabbMin.z = std::min(aabbMin.z, transformedVertices[i + 2]);
            aabbMax.z = std::max(aabbMax.z, transformedVertices[i + 2]);
        }

        // Update the AABB and center/size values
        aabb.min = aabbMin;
        aabb.max = aabbMax;
        
        // Update size and center properties
        sizeX = aabbMax.x - aabbMin.x;
        sizeY = aabbMax.y - aabbMin.y;
        sizeZ = aabbMax.z - aabbMin.z;
        
        centerX = (aabbMin.x + aabbMax.x) * 0.5f;
        centerY = (aabbMin.y + aabbMax.y) * 0.5f;
        centerZ = (aabbMin.z + aabbMax.z) * 0.5f;
    }
    
    // Calculate Oriented Bounding Box (OBB) using object dimensions and model matrix
    void calculateOBB() {
        // Get original dimensions in object space
        glm::vec3 objectDimensions = getObjectSpaceDimensions();
        
        // Calculate half-extents in object space
        glm::vec3 halfExtents = objectDimensions * 0.5f;
        
        // Define the 8 corners of the object-space OBB relative to center
        glm::vec4 corners[8] = {
            {-halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f}, // 0: left-bottom-back
            { halfExtents.x, -halfExtents.y, -halfExtents.z, 1.0f}, // 1: right-bottom-back
            { halfExtents.x, -halfExtents.y,  halfExtents.z, 1.0f}, // 2: right-bottom-front
            {-halfExtents.x, -halfExtents.y,  halfExtents.z, 1.0f}, // 3: left-bottom-front
            {-halfExtents.x,  halfExtents.y, -halfExtents.z, 1.0f}, // 4: left-top-back
            { halfExtents.x,  halfExtents.y, -halfExtents.z, 1.0f}, // 5: right-top-back
            { halfExtents.x,  halfExtents.y,  halfExtents.z, 1.0f}, // 6: right-top-front
            {-halfExtents.x,  halfExtents.y,  halfExtents.z, 1.0f}  // 7: left-top-front
        };
        
        // Apply the model matrix to transform the corners to world space
        for (int i = 0; i < 8; ++i) {
            obbCorners[i] = modelMatrix * corners[i];
        }
    }
    
    // Get the center of the original (untransformed) mesh
    glm::vec3 computeOriginalCenter() {
        if (vertices.empty()) return glm::vec3(0.0f);
        
        float minX = vertices[0], maxX = vertices[0];
        float minY = vertices[1], maxY = vertices[1];
        float minZ = vertices[2], maxZ = vertices[2];

        for (size_t i = 0; i < vertices.size(); i += 3) {
            float x = vertices[i];
            float y = vertices[i + 1];
            float z = vertices[i + 2];

            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
            minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);
        }

        return glm::vec3((minX + maxX) * 0.5f, (minY + maxY) * 0.5f, (minZ + maxZ) * 0.5f);
    }
    
    // *** Size and Dimensions Methods ***
    
    // Get the scaled dimensions (tightly coupled to object)
    glm::vec3 getScaledDimensions() const {
        glm::vec3 objectDimensions = getObjectSpaceDimensions();
        return glm::vec3(
            objectDimensions.x * scaleX,
            objectDimensions.y * scaleY,
            objectDimensions.z * scaleZ
        );
    }
    
    // Get center point in world space
    glm::vec3 getCenter() const {
        return glm::vec3(centerX, centerY, centerZ);
    }

    // Get the tight dimensions (scaled but not rotated)
    glm::vec3 getTightDimensions() const {
        return getScaledDimensions();
    }
    
    // Get the AABB size (loose bounds after all transformations)
    glm::vec3 getAABBSize() const {
        return glm::vec3(sizeX, sizeY, sizeZ);
    }
    
    // Get the original dimensions (without any transformations)
    glm::vec3 getOriginalDimensions() const {
        return getObjectSpaceDimensions();
    }
    
    // Get size with all transformations applied
    glm::vec3 getSize() const {
        return getScaledDimensions(); // This returns tight dimensions
    }
    
    // Get volume of the original object
    float getOriginalVolume() const {
        glm::vec3 dims = getObjectSpaceDimensions();
        return dims.x * dims.y * dims.z;
    }
    
    // Get volume with scaling applied
    float getScaledVolume() const {
        glm::vec3 dims = getScaledDimensions();
        return dims.x * dims.y * dims.z;
    }
    
    // *** Mesh Construction/Initialization ***
    
    // Initialize mesh with vertices, colors, and indices
    void init(const std::vector<GLfloat>& vertices, const std::vector<GLfloat>& colors, const std::vector<unsigned int>& indices) {
        if (vertices.size() % 3 != 0) {
            throw std::invalid_argument("Vertices size must be a multiple of 3 (x, y, z components).");
        }
        
        this->vertices = vertices;
        this->transformedVertices = vertices; // Initialize transformed vertices with original vertices
        this->colors = colors;
        this->indices = indices;

        // Ensure indices size is a multiple of 3
        if (this->indices.size() % 3 != 0) {
            size_t properSize = (this->indices.size() / 3) * 3;
            this->indices.resize(properSize);
        }

        // Reset rotation angles
        rotationX = 0.0f;
        rotationY = 0.0f;
        rotationZ = 0.0f;

        // Reset selection state
        isSelected = false;
        selectedFaceIndex = -1;
        
        // Calculate bounding boxes
        calculateAABB();
        calculateOBB();

        // Build faces from indices
        synchronizeFacesAndIndices();
    }

    // Set vertex data
    void setVertices(const std::vector<GLfloat>& vertices) {
        this->vertices = vertices;
        this->transformedVertices = vertices;
    }

    // Set color data
    void setColors(const std::vector<GLfloat>& colors) {
        this->colors = colors;
    }

    // Set index data
    void setIndices(const std::vector<unsigned int>& indices) {
        this->indices = indices;
    }
    
    // *** Face/Triangle Management ***
    
    // Ensure faces match indices (rebuild after changes)
    void synchronizeFacesAndIndices() {
        // Ensure indices size is a multiple of 3
        if (indices.size() % 3 != 0) {
            size_t properSize = (indices.size() / 3) * 3;
            indices.resize(properSize);
        }

        // Rebuild faces from indices
        constructFaces();

        // Verify consistency
        size_t expectedFaces = indices.size() / 3;
        if (faces.size() != expectedFaces) {
            faces.resize(expectedFaces);

            // Reset selection if now invalid
            if (selectedFaceIndex >= static_cast<int>(faces.size())) {
                selectedFaceIndex = -1;
            }
        }
    }

    // Build face objects from indices and transformed vertices
    void constructFaces() {
        faces.clear();

        // Safety check - ensure valid data
        if (indices.empty() || indices.size() % 3 != 0 || transformedVertices.empty()) {
            return;
        }

        // Create faces for each triangle (group of 3 indices)
        for (size_t i = 0; i + 2 < indices.size(); i += 3) {
            try {
                faces.emplace_back(indices[i], indices[i + 1], indices[i + 2], &transformedVertices);
            }
            catch (const std::out_of_range&) {
                // Stop if indices reference invalid vertices
                OutputDebugString(L"Error in constructFaces(): Face vertex index out of bounds\n");
                break;
            }
        }
    }
    
    // *** Appearance and Material Methods ***
    
    // Update color for the entire mesh
    void updateColors(float r, float g, float b) {
        colorR = r;
        colorG = g;
        colorB = b;
        
        // Update per-vertex colors
        for (size_t i = 0; i < colors.size(); i += 3) {
            colors[i] = r;
            colors[i + 1] = g;
            colors[i + 2] = b;
        }
    }

    // Apply scale factors to the mesh
    void applyScale(float newScaleX, float newScaleY, float newScaleZ) {
        scaleX = newScaleX;
        scaleY = newScaleY;
        scaleZ = newScaleZ;
    }
    
    // Set material properties
    void setMaterial(float shine, float alpha, int material) {
        shininess = shine;
        transparency = alpha;
        materialType = material;
        isTransparent = (alpha > 0.01f);
    }

    // *** Mesh Update and Transformation ***

    // Update mesh transformations and recalculate bounds
    void updateMesh() {
        if (vertices.empty()) return;

        // Convert rotation angles to radians
        float rx = glm::radians(rotationX);
        float ry = glm::radians(rotationY);
        float rz = glm::radians(rotationZ);

        // Create rotation matrices
        glm::mat4 rotX = glm::rotate(glm::mat4(1.0f), rx, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotY = glm::rotate(glm::mat4(1.0f), ry, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 rotZ = glm::rotate(glm::mat4(1.0f), rz, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::mat4 rotationMatrix = rotZ * rotY * rotX;
        
        // Create scale matrix
        glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(scaleX, scaleY, scaleZ));

        // Get original center for rotation around center
        glm::vec3 origCenter = computeOriginalCenter();
        
        // Build full model matrix: translate to position, rotate, scale, center around origin
        modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(centerX, centerY, centerZ)) *
            rotationMatrix * scaleMatrix *
            glm::translate(glm::mat4(1.0f), -origCenter);

        // Transform all vertices using the model matrix
        transformedVertices.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); i += 3) {
            glm::vec4 vertex(vertices[i], vertices[i + 1], vertices[i + 2], 1.0f);
            glm::vec4 transformed = modelMatrix * vertex;

            transformedVertices[i] = transformed.x;
            transformedVertices[i + 1] = transformed.y;
            transformedVertices[i + 2] = transformed.z;
        }

        // Update both bounding volumes
        calculateOBB();   // OBB updates first using the model matrix
        calculateAABB();  // AABB updates from transformed vertices

        // Rebuild faces with new transformed vertices
        synchronizeFacesAndIndices();
    }
    
    // *** File Loading ***
    
    // Load mesh from STL file (binary or ASCII)
    bool loadFromSTL(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open STL file: " << filePath << std::endl;
            return false;
        }

        // Extract filename for object naming
        size_t lastSlash = filePath.find_last_of("/\\");
        size_t lastDot = filePath.find_last_of(".");
        if (lastSlash == std::string::npos) lastSlash = 0;
        else lastSlash++; // Skip the slash
        
        if (lastDot > lastSlash) {
            objectName = filePath.substr(lastSlash, lastDot - lastSlash);
        } else {
            objectName = filePath.substr(lastSlash);
        }
        objectType = "ImportedSTL";
        
        // Default color for imported STL (light gray)
        colorR = 0.8f;
        colorG = 0.8f;
        colorB = 0.8f;

        // Read STL header to determine format
        char header[80];
        file.read(header, 80);

        // Read triangle count
        uint32_t numTriangles;
        file.read(reinterpret_cast<char*>(&numTriangles), sizeof(numTriangles));

        // Check if file is binary STL by size
        file.seekg(0, std::ios::end);
        std::streamsize fileSize = file.tellg();
        file.seekg(80 + sizeof(numTriangles), std::ios::beg);

        std::streamsize expectedSize = 80 + sizeof(numTriangles) + numTriangles * (sizeof(float) * 12 + sizeof(uint16_t));
        
        // Clear existing data
        vertices.clear();
        indices.clear();
        colors.clear();
        
        if (fileSize == expectedSize) {
            // Process binary STL
            for (uint32_t i = 0; i < numTriangles; ++i) {
                float normal[3];
                float vertex1[3], vertex2[3], vertex3[3];
                uint16_t attributeByteCount;

                // Read normal vector (skip for now)
                file.read(reinterpret_cast<char*>(normal), sizeof(normal));

                // Read triangle vertices
                file.read(reinterpret_cast<char*>(vertex1), sizeof(vertex1));
                file.read(reinterpret_cast<char*>(vertex2), sizeof(vertex2));
                file.read(reinterpret_cast<char*>(vertex3), sizeof(vertex3));

                // Read attribute byte count (skip for now)
                file.read(reinterpret_cast<char*>(&attributeByteCount), sizeof(attributeByteCount));

                // Add vertices to the mesh
                size_t baseIndex = vertices.size() / 3;
                vertices.insert(vertices.end(), { vertex1[0], vertex1[1], vertex1[2] });
                vertices.insert(vertices.end(), { vertex2[0], vertex2[1], vertex2[2] });
                vertices.insert(vertices.end(), { vertex3[0], vertex3[1], vertex3[2] });

                // Add indices
                indices.insert(indices.end(), { 
                    static_cast<unsigned int>(baseIndex),
                    static_cast<unsigned int>(baseIndex + 1),
                    static_cast<unsigned int>(baseIndex + 2) 
                });

                // Add colors
                colors.insert(colors.end(), { colorR, colorG, colorB });
                colors.insert(colors.end(), { colorR, colorG, colorB });
                colors.insert(colors.end(), { colorR, colorG, colorB });
            }
        }
        else {
            // Process ASCII STL
            file.close();
            file.open(filePath, std::ios::in);
            if (!file.is_open()) {
                std::cerr << "Failed to reopen STL file in ASCII mode: " << filePath << std::endl;
                return false;
            }

            std::string line;
            size_t baseIndex = 0;
            while (std::getline(file, line)) {
                if (line.find("vertex") != std::string::npos) {
                    std::istringstream iss(line);
                    std::string vertexKeyword;
                    float x, y, z;
                    iss >> vertexKeyword >> x >> y >> z;

                    vertices.insert(vertices.end(), { x, y, z });
                    colors.insert(colors.end(), { colorR, colorG, colorB });
                }
                else if (line.find("endfacet") != std::string::npos) {
                    indices.insert(indices.end(), { 
                        static_cast<unsigned int>(baseIndex),
                        static_cast<unsigned int>(baseIndex + 1),
                        static_cast<unsigned int>(baseIndex + 2) 
                    });
                    baseIndex += 3;
                }
            }
        }

        file.close();
        
        // Initialize the mesh with the loaded data
        init(vertices, colors, indices);
        return true;
    }
    
    // *** Rendering Methods ***
    
    // Render the mesh
    void draw() const {
        // Skip rendering if not visible
        if (!isVisible) return;

        // Safety check for face highlighting
        bool canHighlightFace = isSelected &&
            selectedFaceIndex >= 0 &&
            selectedFaceIndex < static_cast<int>(faces.size()) &&
            selectedFaceIndex * 3 + 2 < indices.size();

        // Enable vertex and color arrays
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(3, GL_FLOAT, 0, transformedVertices.data());
        
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(3, GL_FLOAT, 0, colors.data());

        // Setup transparency if enabled
        if (isTransparent) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColor4f(colorR, colorG, colorB, 1.0f - transparency);
        }

        // Enable wireframe mode if requested
        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }

        // Draw with appropriate highlight method
        if (isSelected) {
            // Highlight entire mesh if no specific face is selected
            if (!canHighlightFace) {
                // Draw mesh with normal colors
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());

                // Draw wireframe highlight overlay
                glDisableClientState(GL_COLOR_ARRAY);
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(2.0f);
                glColor3f(1.0f, 1.0f, 0.0f); // Yellow highlight
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glLineWidth(1.0f);
            }
            // Highlight specific triangle
            else {
                // Draw non-selected triangles normally
                for (int i = 0; i < static_cast<int>(faces.size()); ++i) {
                    if (i != selectedFaceIndex && i * 3 + 2 < indices.size()) {
                        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &indices[i * 3]);
                    }
                }

                // Draw the selected triangle with highlight
                glDisableClientState(GL_COLOR_ARRAY);
                
                // Fill with semi-transparent highlight
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glColor4f(1.0f, 0.5f, 0.0f, 0.7f); // Orange highlight
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &indices[selectedFaceIndex * 3]);

                // Draw outline
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glLineWidth(3.0f);
                glColor3f(1.0f, 0.8f, 0.0f); // Yellow-orange outline
                glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, &indices[selectedFaceIndex * 3]);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glLineWidth(1.0f);
                glDisable(GL_BLEND);
            }
        }
        else {
            // Draw normally without highlighting
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data());
        }

        // Restore render states
        if (isTransparent) {
            glDisable(GL_BLEND);
        }

        if (wireframeMode) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);

        // Draw additional visualizations if enabled
        if (showBoundingBox) {
            drawBoundingBox();
        }

        if (showVertices) {
            drawVertices();
        }
    }

    // Draw local coordinate axes
    void drawLocalAxis() const {
        if (vertices.empty() || !isVisible) return;
        
        // Draw the axes
        glBegin(GL_LINES);

        // X-axis (red)
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX + 2.0f, centerY, centerZ);

        // Y-axis (green)
        glColor3f(0.0f, 1.0f, 0.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX, centerY + 2.0f, centerZ);

        // Z-axis (blue)
        glColor3f(0.0f, 0.0f, 1.0f);
        glVertex3f(centerX, centerY, centerZ);
        glVertex3f(centerX, centerY, centerZ + 2.0f);

        glEnd();
    }
    
    // Draw the bounding box (using OBB corners)
    void drawBoundingBox() const {
        if (transformedVertices.empty() || !isVisible) return;

        // Draw using the OBB corners for accurate visualization
        glColor3f(1.0f, 1.0f, 0.0f); // Yellow color
        glBegin(GL_LINES);

        // Bottom face
        glVertex3f(obbCorners[0].x, obbCorners[0].y, obbCorners[0].z);
        glVertex3f(obbCorners[1].x, obbCorners[1].y, obbCorners[1].z);

        glVertex3f(obbCorners[1].x, obbCorners[1].y, obbCorners[1].z);
        glVertex3f(obbCorners[2].x, obbCorners[2].y, obbCorners[2].z);

        glVertex3f(obbCorners[2].x, obbCorners[2].y, obbCorners[2].z);
        glVertex3f(obbCorners[3].x, obbCorners[3].y, obbCorners[3].z);

        glVertex3f(obbCorners[3].x, obbCorners[3].y, obbCorners[3].z);
        glVertex3f(obbCorners[0].x, obbCorners[0].y, obbCorners[0].z);

        // Top face
        glVertex3f(obbCorners[4].x, obbCorners[4].y, obbCorners[4].z);
        glVertex3f(obbCorners[5].x, obbCorners[5].y, obbCorners[5].z);

        glVertex3f(obbCorners[5].x, obbCorners[5].y, obbCorners[5].z);
        glVertex3f(obbCorners[6].x, obbCorners[6].y, obbCorners[6].z);

        glVertex3f(obbCorners[6].x, obbCorners[6].y, obbCorners[6].z);
        glVertex3f(obbCorners[7].x, obbCorners[7].y, obbCorners[7].z);

        glVertex3f(obbCorners[7].x, obbCorners[7].y, obbCorners[7].z);
        glVertex3f(obbCorners[4].x, obbCorners[4].y, obbCorners[4].z);

        // Vertical edges
        glVertex3f(obbCorners[0].x, obbCorners[0].y, obbCorners[0].z);
        glVertex3f(obbCorners[4].x, obbCorners[4].y, obbCorners[4].z);

        glVertex3f(obbCorners[1].x, obbCorners[1].y, obbCorners[1].z);
        glVertex3f(obbCorners[5].x, obbCorners[5].y, obbCorners[5].z);

        glVertex3f(obbCorners[2].x, obbCorners[2].y, obbCorners[2].z);
        glVertex3f(obbCorners[6].x, obbCorners[6].y, obbCorners[6].z);

        glVertex3f(obbCorners[3].x, obbCorners[3].y, obbCorners[3].z);
        glVertex3f(obbCorners[7].x, obbCorners[7].y, obbCorners[7].z);

        glEnd();
    }
    
    // Draw points at each vertex
    void drawVertices() const {
        if (transformedVertices.empty() || !isVisible) return;

        glPointSize(5.0f);
        glBegin(GL_POINTS);
        glColor3f(0.0f, 0.0f, 0.0f); // Black dots

        for (size_t i = 0; i < transformedVertices.size(); i += 3) {
            glVertex3f(transformedVertices[i], transformedVertices[i + 1], transformedVertices[i + 2]);
        }

        glEnd();
    }
    
    // *** Toggle Methods ***
    
    // Toggle bounding box visibility
    void toggleBoundingBox() {
        showBoundingBox = !showBoundingBox;
    }

    // Toggle vertex points visibility
    void toggleVertices() {
        showVertices = !showVertices;
    }
    
    // Toggle wireframe rendering mode
    void toggleWireframe() {
        wireframeMode = !wireframeMode;
    }
    
    // Toggle mesh visibility
    void toggleVisibility() {
        isVisible = !isVisible;
    }
    
    // *** Selection Methods ***
    
    // Set or clear selection state
    void setSelected(bool selected) {
        isSelected = selected;
        if (!selected) {
            selectedFaceIndex = -1; // Clear face selection when deselecting
        }
    }

    // Select a specific face/triangle
    void selectFace(int faceIndex) {
        if (faceIndex >= 0 && faceIndex < static_cast<int>(faces.size()) &&
            faceIndex * 3 + 2 < indices.size()) {
            selectedFaceIndex = faceIndex;
        }
        else {
            selectedFaceIndex = -1; // Invalid face index
        }
    }

    // Check if a specific face is selected
    bool hasFaceSelected() const {
        return isSelected && selectedFaceIndex >= 0;
    }
};
