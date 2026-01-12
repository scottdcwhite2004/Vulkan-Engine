#pragma once
#include <GLFW/glfw3.h>
#include <unordered_map>
class InputManager final
{
	static std::unordered_map<int, bool> _keyStates;
	static std::unordered_map<int, bool> _mouseButtonStates;
	static double _mouseX, _mouseY;



	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos);

public:
	InputManager() = default;
	~InputManager() = default;
	static void Init(GLFWwindow* window);

	inline static bool isKeyPressed(int key)
	{
		return _keyStates[key];
	}
	static bool isKeyReleased(int key);

	inline static bool isMouseButtonPressed(int button)
	{
		return _mouseButtonStates[button];
	}
	static bool isMouseButtonReleased(int button);

	inline static double getMouseX()
		{
		return _mouseX;
	}
	inline static double getMouseY()
		{
		return _mouseY;
	}
};

