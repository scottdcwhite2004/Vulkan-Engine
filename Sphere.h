#pragma once
#include "Shape.h"
#include "Vertex.h"
#include "glm/glm.hpp"
#include <vector>

class Sphere :
    public Shape
{
    std::vector<Vertex> _localVertices;
    std::vector<uint16_t> _localIndices;
    float _radius;

public:
    Sphere(const glm::vec3& position, const Material& material, const float& radius) : Shape(position, material), _radius(radius) {};
	Sphere() = default;
    ~Sphere();
    void create() override;
	void move() override;
	bool WithinBounds(const glm::vec3& point, float buffer = 0.0f) const;

};

