#pragma once

class Ray {
	public:
	glm::vec3 origin; // Ray origin
	glm::vec3 direction; // Ray direction
	float tMin; // Minimum distance to consider for intersection
	float tMax; // Maximum distance to consider for intersection
	Ray(const glm::vec3& origin, const glm::vec3& direction, float tMin = 0.0f, float tMax = std::numeric_limits<float>::max())
		: origin(origin), direction(glm::normalize(direction)), tMin(tMin), tMax(tMax) {}
	// Function to get a point along the ray at distance t
	glm::vec3 getPoint(float t) const {
		return origin + direction * t;
	}
};