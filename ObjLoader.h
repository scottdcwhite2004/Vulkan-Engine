#pragma once
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>

struct  object
{
	std::vector<uint16_t> vertexIndices;
	std::vector<uint16_t> textureIndices;
	std::vector<uint16_t> normalIndices;
	std::string objectNames;
	int verticesPerFace;
};

struct Coordinates {
	float x, y, z;
};

struct ObjTexCoord {
	float u, v;
};

struct Normal {
	float x, y, z;
};

class ObjLoader final
{
public:
	ObjLoader() = default;
	~ObjLoader() = default;
	std::tuple<std::vector<Coordinates>,std::vector<ObjTexCoord>,std::vector<Normal>, std::vector<uint16_t>, std::vector<uint16_t>, std::vector<uint16_t>> loadFile(const std::string& filename) const;

};

