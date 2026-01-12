#include "Sphere.h"

void Sphere::create() {
	const int latitudeBands = 18;
	const int longitudeBands = 36;
	// Create vertices
	for (int latNumber = 0; latNumber <= latitudeBands; ++latNumber) {
		const float theta = latNumber * 3.14159265f / latitudeBands;
		const float sinTheta = sin(theta);
		const float cosTheta = cos(theta);
		for (int longNumber = 0; longNumber <= longitudeBands; ++longNumber) {
			const float phi = longNumber * 2.0f * 3.14159265f / longitudeBands;
			const float sinPhi = sin(phi);
			const float cosPhi = cos(phi);
			const float x = cosPhi * sinTheta;
			const float y = cosTheta;
			const float z = sinPhi * sinTheta;
			const float u = 1.0f - (static_cast<float>(longNumber) / longitudeBands);
			const float v = 1.0f - (static_cast<float>(latNumber) / latitudeBands);
			glm::vec3 normal = glm::normalize(glm::vec3(x, y, z));
			_localVertices.push_back({ {glm::vec3(_radius * x, _radius * y, _radius * z) + getPos()}, {1,1,1}, {u, v}, normal});
		}
	}
	// Create indices
	for (int latNumber = 0; latNumber < latitudeBands; ++latNumber) {
		for (int longNumber = 0; longNumber < longitudeBands; ++longNumber) {
			int first = (latNumber * (longitudeBands + 1)) + longNumber;
			int second = first + longitudeBands + 1;
			_localIndices.push_back(first);
			_localIndices.push_back(second);
			_localIndices.push_back(first + 1);
			_localIndices.push_back(second);
			_localIndices.push_back(second + 1);
			_localIndices.push_back(first + 1);
		}
	}
	setVertices(_localVertices);
	setIndices(_localIndices);
}


void Sphere::move() {
	// Placeholder for moving the sphere
	// Actual implementation would involve updating the sphere's position
}

Sphere::~Sphere() = default;

bool Sphere::WithinBounds(const glm::vec3& point, float buffer) const {
	const glm::vec3 center = getPos();
	const float distanceSquared = glm::dot(point - center, point - center);
	const float effectiveRadius = _radius + buffer;
	return distanceSquared <= (effectiveRadius * effectiveRadius);
}