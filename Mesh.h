#pragma once
#include "Shape.h"
#include "Vertex.h"
#include "ObjLoader.h"
#include "glm/glm.hpp"
#include "RenderContext.h"
#include <vector>
#include <string>

class Mesh :
    public Shape
{
    std::string _filePath;
	std::vector<Vertex> _localVertices;
	std::vector<uint16_t> _localIndices;

    public:
        Mesh(const glm::vec3& position, const Material& material, const std::string& filePath) : Shape(position, material), _filePath(filePath) {};
		Mesh() = default;
        ~Mesh();
        void create() override;
		void move() override;
		const std::string getModelPath() const { return _filePath; }
};

