#pragma once
#include <vulkan/vulkan.h>

struct RenderContext
{
	VkDevice device{};
	VkPhysicalDevice physicalDevice{};
	VkQueue graphicsQueue{};
	VkCommandPool commandPool{};
	VkDescriptorSetLayout descriptorSetLayout{};
	VkDescriptorPool descriptorPool{};

	RenderContext& operator=(const RenderContext&) = default;

	RenderContext(const RenderContext&) = default;

	RenderContext(VkDevice pDevice, VkPhysicalDevice pPhysicalDevice, VkQueue pGraphicsQueue, VkCommandPool pCommandPool, VkDescriptorSetLayout pDescriptorSetLayout, VkDescriptorPool pDescriptorPool)
		: device(pDevice), physicalDevice(pPhysicalDevice), graphicsQueue(pGraphicsQueue), commandPool(pCommandPool), descriptorSetLayout(pDescriptorSetLayout), descriptorPool(pDescriptorPool)
	{
	}

	RenderContext() = default;
};

