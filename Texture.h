#pragma once
#include "vulkan/vulkan.h"
#include <stb_image.h>
#include <stdexcept>
#include "Image.h"
#include <string>

class Texture final
{

	std::string _texturePath;

	Image _image;

	VkDevice _device{ VK_NULL_HANDLE };
	VkPhysicalDevice _physicalDevice{ VK_NULL_HANDLE };
	VkCommandPool _commandPool{ VK_NULL_HANDLE }; // if needed in ctor via Image
	VkQueue _graphicsQueue{ VK_NULL_HANDLE };     // if needed in ctor via Image

	VkImage _textureImage{ VK_NULL_HANDLE };
	VkImageView _textureImageView{ VK_NULL_HANDLE };
	VkDeviceMemory _textureImageMemory{ VK_NULL_HANDLE };
	VkSampler _textureSampler{ VK_NULL_HANDLE };

	public:
		Texture() = default;
		~Texture() = default;

		Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, const std::string& texturePath)
			: _texturePath(texturePath)
			, _image(device, physicalDevice, commandPool, graphicsQueue)
			, _device(device)
			, _physicalDevice(physicalDevice)
			, _commandPool(commandPool)
			, _graphicsQueue(graphicsQueue)
		{
			createTextureImage();
			createTextureImageView();
			createTextureSampler();
		}

		Texture(const Texture& other) = default;
		Texture& operator=(const Texture& other) = default;
		Texture(Texture&& other) = default;
		Texture& operator=(Texture&& other) = default;

		void createTextureImage();
		void createTextureImageView();
		void createTextureSampler();
		void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		VkImageView getTextureImageView() const { return _textureImageView; }
		VkSampler getTextureSampler() const { return _textureSampler; }
		void destroy()
		{
			if (_device == VK_NULL_HANDLE) return;

			if (_textureSampler != VK_NULL_HANDLE) {
				vkDestroySampler(_device, _textureSampler, nullptr);
				_textureSampler = VK_NULL_HANDLE;
			}
			if (_textureImageView != VK_NULL_HANDLE) {
				vkDestroyImageView(_device, _textureImageView, nullptr);
				_textureImageView = VK_NULL_HANDLE;
			}
			if (_textureImage != VK_NULL_HANDLE) {
				vkDestroyImage(_device, _textureImage, nullptr);
				_textureImage = VK_NULL_HANDLE;
			}
			if (_textureImageMemory != VK_NULL_HANDLE) {
				vkFreeMemory(_device, _textureImageMemory, nullptr);
				_textureImageMemory = VK_NULL_HANDLE;
			}
		}
};

