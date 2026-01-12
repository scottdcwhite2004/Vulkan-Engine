#pragma once
#include "Shape.h"
#include "Vertex.h"
#include "glm/glm.hpp"
#include <vector>

class Cube :
    public Shape
{
    
    public:
		Cube(const glm::vec3& position, const Material& material) : Shape(position, material) {};
		Cube() = default;
		~Cube();

		void create() override;
		void move() override;
};

