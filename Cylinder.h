#pragma once
#include "Shape.h"
#include "Vertex.h"
#include "glm/glm.hpp"
#include <vector>
class Cylinder :
    public Shape
{
    std::vector<Vertex> _localVertices;
    std::vector<uint16_t> _localIndices;
	float _radius;
    float _height;
    int _segments;

public:
     
    Cylinder(const glm::vec3& position, const Material& material, const float& radius, const float& height, const int& segments) : Shape(position, material), _radius(radius), _height(height), _segments(segments) {};
	Cylinder() = default;
    ~Cylinder();

    void create() override;
    void move() override;

};

