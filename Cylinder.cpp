#include "Cylinder.h"

void Cylinder::create() {
	// Create vertices
	for (int i = 0; i <= _segments; ++i) {
		const float theta = static_cast<float>(i) / _segments * 2.0f * 3.14159265f;
		const float cosTheta = cos(theta);
		const float sinTheta = sin(theta);
		const float x = _radius * cos(theta);
		const float z = _radius * sin(theta);

		glm::vec3 normal = glm::normalize(glm::vec3(cosTheta, 0.0f, sinTheta));
		// Bottom circle
		_localVertices.push_back({ {glm::vec3(x, -_height / 2, z) + getPos()}, {1,1,1}, {static_cast<float>(i) / _segments, 0}, normal });
		// Top circle
		_localVertices.push_back({ {glm::vec3(x, _height / 2, z) + getPos()}, {1,1,1}, {static_cast<float>(i) / _segments, 1}, normal});
	}
	// Create indices
	for (int i = 0; i < _segments; ++i) {
		int bottom1 = i * 2;
		int top1 = bottom1 + 1;
		int bottom2 = ((i + 1) % _segments) * 2;
		int top2 = bottom2 + 1;
		// Side face
		_localIndices.push_back(bottom1);
		_localIndices.push_back(top1);
		_localIndices.push_back(top2);
		_localIndices.push_back(top2);
		_localIndices.push_back(bottom2);
		_localIndices.push_back(bottom1);
	}

	const uint16_t baseCenterIndex = static_cast<uint16_t>(_localVertices.size());
	_localVertices.push_back({ {glm::vec3(0.0f, -_height / 2.0f, 0.0f) + getPos()}, {1,1,1}, {0.5f,0.5f}, {0.0f,-1.0f,0.0f}});
	const uint16_t topCenterIndex = static_cast<uint16_t>(_localVertices.size());
	_localVertices.push_back({ {glm::vec3(0.0f,  _height / 2.0f, 0.0f) + getPos()}, {1,1,1}, {0.5f,0.5f}, {0.0f, 1.0f,0.0f}});

	for (int i = 0; i < _segments; ++i) {
		const uint16_t b0 = static_cast<uint16_t>(i * 2);
		const uint16_t b1 = static_cast<uint16_t>(((i + 1) % _segments) * 2);
		const uint16_t t0 = static_cast<uint16_t>(b0 + 1);
		const uint16_t t1 = static_cast<uint16_t>(b1 + 1);

		// Bottom cap (fan) - normal down
		_localIndices.push_back(baseCenterIndex);
		_localIndices.push_back(b1);
		_localIndices.push_back(b0);

		// Top cap (fan) - normal up
		_localIndices.push_back(topCenterIndex);
		_localIndices.push_back(t0);
		_localIndices.push_back(t1);
	}
	setVertices(_localVertices);
	setIndices(_localIndices);
}


void Cylinder::move() {
	// Placeholder for moving the cylinder
	// Actual implementation would involve updating the cylinder's position
}

Cylinder::~Cylinder() = default;

