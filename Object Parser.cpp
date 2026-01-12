#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <tuple>

struct  object
{
	std::vector<int> vertexIndices;
	std::vector<int> textureIndices;
	std::vector<int> normalIndices;
	std::string objectNames;
	int verticesPerFace;
};

struct Vertex {
	float x, y, z;
};

struct Texture {
	float u, v;
};

struct Normal {
	float x, y, z;
};

std::vector<Vertex> ParseOBJ(const std::string& filename)
{

	std::ifstream fin(filename);

	std::vector<object> obj;
	std::vector<Vertex> vertices;
	std::vector<Texture> textures;
	std::vector<Normal> normals;


	//This is related to loading multiple objects
	int objectCount = 0;


	std::string tag;
	while (fin >> tag) {


		if (tag == "o")
		{
			std::string objectName;
			fin >> objectName;
			obj.push_back(object());
			obj[objectCount].objectNames = objectName;
			objectCount++;
		}
		else if (tag == "v")
		{
			Vertex vertex;
			fin >> vertex.x >> vertex.y >> vertex.z;
			vertices.push_back(vertex);
		}
		else if (tag == "vt")
		{
			Texture texture;
			fin >> texture.u >> texture.v;
			textures.push_back(texture);
		}
		else if (tag == "vn")
		{
			Normal normal;
			fin >> normal.x >> normal.y >> normal.z;
			normals.push_back(normal);
		}
		else if (tag == "f")
		{
			auto lineStart = fin.tellg();
			int count = 0;
			for (char c = ' '; c != 'f' && !fin.eof(); fin >> c) {
				if (c == '/') {
					count++;
				}
			}
			fin.seekg(lineStart);
			obj[objectCount - 1].verticesPerFace = count / 2;

			for (int i = 0; i < count / 2; i++) {

				char ch;
				int vIndex, tIndex, nIndex;
				fin >> vIndex >> ch >> tIndex >> ch >> nIndex;
				obj[objectCount - 1].vertexIndices.push_back(vIndex);
				obj[objectCount - 1].textureIndices.push_back(tIndex);
				obj[objectCount - 1].normalIndices.push_back(nIndex);
			}
		}

	}

	return vertices;
}





