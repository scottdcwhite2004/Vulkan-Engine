#include "Mesh.h"



void Mesh::create() {
    ObjLoader loader;
    auto [coords, textures, normals, faceIndices, textureIndices, normalIndices] = loader.loadFile(_filePath);

    _localVertices.clear();
    _localVertices.reserve(faceIndices.size());

    for (size_t i = 0; i < faceIndices.size(); ++i) {
        const uint16_t vi = faceIndices[i];
		const uint16_t ti = textureIndices[i];
		const uint16_t ni = normalIndices[i];

        Vertex v{};
		v.pos = glm::vec3(coords[vi].x, coords[vi].y, coords[vi].z) + getPos();
		v.texCoord = glm::vec2(textures[ti].u, 1.0f - textures[ti].v);
		v.normal = glm::vec3(normals[ni].x, normals[ni].y, normals[ni].z);
		v.color = glm::vec3(1.0f, 1.0f, 1.0f); // Default color; can be modified later

        _localVertices.push_back(v);
    }

    _localIndices.resize(static_cast<uint16_t>(_localVertices.size()));
    for (uint16_t i = 0; i < static_cast<uint16_t>(_localIndices.size()); ++i) {
        _localIndices[i] = i;
    }

    setVertices(_localVertices);
    setIndices(_localIndices);
}

void Mesh::move() {
	// Placeholder for moving the mesh
	// Actual implementation would involve updating the mesh's position
}

Mesh::~Mesh() = default;
