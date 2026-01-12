#pragma once
#include "glm/glm.hpp"
class GraphicsObject
{
	glm::vec3 _position;

	public:
		inline glm::vec3 getPos() const { return _position; }
		inline void setPos(const glm::vec3& position) {
			_position = position;
		}
		explicit GraphicsObject(const glm::vec3& position) : _position(position) {};
		GraphicsObject() = default;
		virtual ~GraphicsObject();
};

