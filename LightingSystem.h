#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include "Light.h"
#include "glm/glm.hpp"
#include "RenderContext.h"

class LightingSystem final
{
	std::vector<VkBuffer> _buffers;
	std::vector<VkDeviceMemory> _memories;
	std::vector<void*> _mapped;
	std::vector<VkDescriptorBufferInfo> _descriptorInfos;

	uint32_t _framesInFlight{ 0 };

	static uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags props);

	void createBuffer(const RenderContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;

public:
	
	LightingSystem() = default;
	~LightingSystem() = default;

	void create(const RenderContext& ctx, uint32_t framesInFlight);
	void destroy(const RenderContext& ctx);

	void update(uint32_t frameIndex, const glm::vec3& viewPosWorld, float shininess, const std::vector<Light>& sceneLights);

	const VkDescriptorBufferInfo& descriptorInfo(uint32_t frameIndex) const { return _descriptorInfos[frameIndex]; }
	const std::vector<VkDescriptorBufferInfo> descriptorInfos() const { return _descriptorInfos; }
	uint32_t framesInFlight() const { return _framesInFlight; }

};

