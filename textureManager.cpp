#include "textureManager.h"

textureManager::textureManager(textureManager&& other) noexcept
	: _textures(std::move(other._textures))
	, _loadedTextures(std::move(other._loadedTextures))
	, _device(other._device)
	, _physicalDevice(other._physicalDevice)
	, _commandPool(other._commandPool)
	, _graphicsQueue(other._graphicsQueue)
	, _activeIndex(other._activeIndex)
{
	other._device = VK_NULL_HANDLE;
	other._physicalDevice = VK_NULL_HANDLE;
	other._commandPool = VK_NULL_HANDLE;
	other._graphicsQueue = VK_NULL_HANDLE;
	other._activeIndex = -1;
}

textureManager& textureManager::operator=(textureManager&& other) noexcept
{
	if (this == &other)
	{
		return *this;
	}

	// Release current resources before taking ownership
	destroy();

	_textures = std::move(other._textures);
	_loadedTextures = std::move(other._loadedTextures);
	_device = other._device;
	_physicalDevice = other._physicalDevice;
	_commandPool = other._commandPool;
	_graphicsQueue = other._graphicsQueue;
	_activeIndex = other._activeIndex;

	other._device = VK_NULL_HANDLE;
	other._physicalDevice = VK_NULL_HANDLE;
	other._commandPool = VK_NULL_HANDLE;
	other._graphicsQueue = VK_NULL_HANDLE;
	other._activeIndex = -1;

	return *this;
}

void textureManager::destroy()
{
	for (const auto& kv : _textures) {
		if (kv.second) {
			kv.second->destroy();
		}
	}
	_loadedTextures.clear();
	_textures.clear();
}
