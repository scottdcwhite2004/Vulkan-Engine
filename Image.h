#pragma once
#include "vulkan/vulkan.h"
#include <stdexcept>
class Image final
{
	VkQueue _graphicsQueue;
	VkCommandPool _commandPool;
	VkPhysicalDevice _physicalDevice;
	VkDevice _device;


	public:
		Image() = default;
		Image(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue)
			: _graphicsQueue(graphicsQueue)
			, _commandPool(commandPool)
			, _physicalDevice(physicalDevice)
			, _device(device)
		{
		}

		Image(const Image& other) = default;


		Image& operator=(const Image& other) = default;

		~Image() = default;
		void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
		void transitionImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void transitionDepthImageLayout(VkImage& image, VkImageLayout oldLayout, VkImageLayout newLayout);
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
		VkCommandBuffer beginSingleTimeCommands();
		void endSingleTimeCommands(VkCommandBuffer commandBuffer);
		uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		VkDevice device() const { return _device; }
};

