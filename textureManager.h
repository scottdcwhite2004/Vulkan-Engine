#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include "Texture.h"
class textureManager final
{
	std::unordered_map<std::string, std::unique_ptr<Texture>> _textures;
	std::vector<Texture*> _loadedTextures;
	VkDevice _device;
	VkPhysicalDevice _physicalDevice;
	VkCommandPool _commandPool;
	VkQueue _graphicsQueue;

	int _activeIndex{ -1 };

public:
	textureManager() = default;
	~textureManager() = default;

	// Non-copyable: owning unique_ptr and GPU resources cannot be copied safely
	textureManager(const textureManager&) = delete;
	textureManager& operator=(const textureManager&) = delete;

	// Movable: transfer ownership
	textureManager(textureManager&& other) noexcept;
	textureManager& operator=(textureManager&& other) noexcept;

	void initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue)
	{
		_device = device;
		_physicalDevice = physicalDevice;
		_commandPool = commandPool;
		_graphicsQueue = graphicsQueue;
	}

	Texture* addTexture(const std::string& name, const std::string& texturePath)
	{
		if (_device == VK_NULL_HANDLE || _physicalDevice == VK_NULL_HANDLE || _commandPool == VK_NULL_HANDLE || _graphicsQueue == VK_NULL_HANDLE) {
			throw std::runtime_error("textureManager not initialized: call initialize(...) before addTexture.");
		}

		auto tex = std::make_unique<Texture>(_device, _physicalDevice, _commandPool, _graphicsQueue, texturePath);
		Texture* const raw = tex.get();
		_loadedTextures.push_back(raw);
		_textures.emplace(name, std::move(tex));
		if (_activeIndex < 0) { _activeIndex = 0; }
		return raw;
	}

	Texture* getTexture(const std::string& name) const
	{
		const auto it = _textures.find(name);
		return it == _textures.end() ? nullptr : it->second.get();
	}

	void destroy();
};


