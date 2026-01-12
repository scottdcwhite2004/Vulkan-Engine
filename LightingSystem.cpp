#include "LightingSystem.h"
#include <cstring>
#include <stdexcept>

uint32_t LightingSystem::findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags props)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(phys, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & props) == props) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

void LightingSystem::createBuffer(const RenderContext& ctx, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory) const {
	VkBufferCreateInfo info{ };
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.size = size;
	info.usage = usage;
	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(ctx.device, &info, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("LightingSystem: vkCreateBuffer failed");

	VkMemoryRequirements req{};
	vkGetBufferMemoryRequirements(ctx.device, buffer, &req);

	VkMemoryAllocateInfo alloc{};
	alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc.allocationSize = req.size;
	alloc.memoryTypeIndex = findMemoryType(ctx.physicalDevice, req.memoryTypeBits, properties);

	if (vkAllocateMemory(ctx.device, &alloc, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("LightingSystem: vkAllocateMemory failed");

	vkBindBufferMemory(ctx.device, buffer, memory, 0);
}

void LightingSystem::create(const RenderContext& ctx, uint32_t framesInFlight) {
	_framesInFlight = framesInFlight;

	const VkDeviceSize size = sizeof(LightingUBO);
	_buffers.resize(framesInFlight);
	_memories.resize(framesInFlight);
	_mapped.resize(framesInFlight);
	_descriptorInfos.resize(framesInFlight);

	for (uint32_t i = 0; i < framesInFlight; ++i)
	{
		createBuffer(ctx,
			size,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			_buffers[i],
			_memories[i]);

		if (vkMapMemory(ctx.device, _memories[i], 0, size, 0, &_mapped[i]) != VK_SUCCESS || _mapped[i] == nullptr)
			throw std::runtime_error("LightingSystem: vkMapMemory failed");

		_descriptorInfos[i].buffer = _buffers[i];
		_descriptorInfos[i].offset = 0;
		_descriptorInfos[i].range = size;
	}
}

void LightingSystem::destroy(const RenderContext& ctx) {
	for (uint32_t i = 0; i < _framesInFlight; ++i)
	{
		if (_mapped[i] != nullptr)
		{
			vkUnmapMemory(ctx.device, _memories[i]);
			_mapped[i] = nullptr;
		}
		if (_buffers[i] != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(ctx.device, _buffers[i], nullptr);
			_buffers[i] = VK_NULL_HANDLE;
		}
		if (_memories[i] != VK_NULL_HANDLE)
		{
			vkFreeMemory(ctx.device, _memories[i], nullptr);
			_memories[i] = VK_NULL_HANDLE;
		}
	}
	_buffers.clear();
	_memories.clear();
	_mapped.clear();
	_descriptorInfos.clear();
	_framesInFlight = 0;
}

void LightingSystem::update(uint32_t frameIndex, const glm::vec3& viewPosWorld, float shininess, const std::vector<Light>& sceneLights) {
	LightingUBOCPU ubo{};
	ubo.viewPosWorld = viewPosWorld;
	ubo.shininess = shininess;

	const int count = static_cast<int>(std::min<std::size_t>(sceneLights.size(), LightingUBO::MaxLights));
	ubo.lightCount = count;

	for (int i = 0; i < count; ++i) {
		ubo.lights[i] = sceneLights[i].toGPULight();
	}

	std::memcpy(_mapped[frameIndex], &ubo, sizeof(ubo));

}


