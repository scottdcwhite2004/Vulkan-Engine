#include "InputManager.h"

std::unordered_map<int, bool> InputManager::_keyStates;
std::unordered_map<int, bool> InputManager::_mouseButtonStates;
double InputManager::_mouseX = 0.0;
double InputManager::_mouseY = 0.0;


bool InputManager::isKeyReleased(int key)
{
	return !_keyStates[key];
}


bool InputManager::isMouseButtonReleased(int button)
{
	return !_mouseButtonStates[button];
}

void InputManager::Init(GLFWwindow* window)
{
	glfwSetKeyCallback(window, keyCallback);
	glfwSetMouseButtonCallback(window, mouseButtonCallback);
	glfwSetCursorPosCallback(window, cursorPositionCallback);
}

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		_keyStates[key] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		_keyStates[key] = false;
	}
}

void InputManager::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		_mouseButtonStates[button] = true;
	}
	else if (action == GLFW_RELEASE)
	{
		_mouseButtonStates[button] = false;
	}
}

void InputManager::cursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	_mouseX = xpos;
	_mouseY = ypos;
}

