#include "particleSystem.h"
#include <random>
#include <stdexcept>

namespace {
    uint32_t findMemoryType(VkPhysicalDevice phys, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(phys, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1u << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("particleSystem: failed to find suitable memory type");
    }

    VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool pool)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = pool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        if (vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("particleSystem: vkAllocateCommandBuffers failed");
        }

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("particleSystem: vkBeginCommandBuffer failed");
        }

        return commandBuffer;
    }

    void endSingleTimeCommands(VkDevice device, VkQueue queue, VkCommandPool pool, VkCommandBuffer commandBuffer)
    {
        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("particleSystem: vkEndCommandBuffer failed");
        }

        // Use a fence to wait for this single submission, avoiding full queue idle
        const VkFenceCreateInfo fenceInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        VkFence fence = VK_NULL_HANDLE;
        if (vkCreateFence(device, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
            throw std::runtime_error("particleSystem: vkCreateFence failed");
        }

        VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        if (vkQueueSubmit(queue, 1, &submitInfo, fence) != VK_SUCCESS) {
            vkDestroyFence(device, fence, nullptr);
            throw std::runtime_error("particleSystem: vkQueueSubmit failed");
        }

        vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(device, fence, nullptr);

        vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
    }

    void copyBuffer(VkDevice device, VkQueue queue, VkCommandPool pool, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
    {
        const VkCommandBuffer cmd = beginSingleTimeCommands(device, pool);
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(cmd, srcBuffer, dstBuffer, 1, &copyRegion);
        endSingleTimeCommands(device, queue, pool, cmd);
    }

    void createBuffer(VkDevice device,
        VkPhysicalDevice phys,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& bufferMemory)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("particleSystem: failed to create buffer");

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(phys, memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
            throw std::runtime_error("particleSystem: failed to allocate buffer memory");

        vkBindBufferMemory(device, buffer, bufferMemory, 0);
    }
}

void particleSystem::spawnBurst(uint32_t count, float speedMin, float speedMax)
{
    if (count == 0 || _maxParticles == 0) return;
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> ang(0.0f, 6.2831853f);
    std::uniform_real_distribution<float> elev(-0.5f, 0.5f);
    std::uniform_real_distribution<float> spd(speedMin, speedMax);
    std::uniform_real_distribution<float> life(1.0f, 4.0f);

    for (uint32_t i = 0; i < count && _particles.size() < _maxParticles; ++i)
    {
        const float a = ang(rng);
        const float e = elev(rng);
        const glm::vec3 dir = glm::normalize(glm::vec3(std::cos(a), e, std::sin(a)));
        Particle p{};
        p.position = _origin;
        p.velocity = dir * spd(rng);
        p.maxLifetime = life(rng);
        p.lifetime = p.maxLifetime;
        _particles.push_back(p);
    }
    _activeParticles = static_cast<uint32_t>(_particles.size());
}

void particleSystem::update(float deltaTime)
{
    if (_particles.empty()) return;

    // Simple Euler integration + damping
    const glm::vec3 gravity(0.0f, -9.8f, 0.0f);
    const float damping = 0.98f;

    for (auto& p : _particles)
    {
        if (p.lifetime <= 0.0f) continue;
        p.velocity += gravity * deltaTime;
        p.velocity *= damping;
        p.position += p.velocity * deltaTime;
        p.lifetime -= deltaTime;
    }

    // remove dead by compacting
    size_t write = 0;
    for (size_t read = 0; read < _particles.size(); ++read)
    {
        if (_particles[read].lifetime > 0.0f)
        {
            if (write != read) _particles[write] = _particles[read];
            ++write;
        }
    }
    _particles.resize(write);
    _activeParticles = static_cast<uint32_t>(_particles.size());
}

void particleSystem::ensureGPUBuffer(const RenderContext& ctx)
{
    if (_instanceBuffer != VK_NULL_HANDLE) return;

    _device = ctx.device;
    _physicalDevice = ctx.physicalDevice;
    _commandPool = ctx.commandPool;
    _graphicsQueue = ctx.graphicsQueue;

    const VkDeviceSize size = sizeof(Particle) * _maxParticles;

    createBuffer(_device, _physicalDevice, size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        _instanceBuffer, _instanceMemory);

}

void particleSystem::uploadInstances(const RenderContext& ctx)
{
    if (_activeParticles == 0 || _instanceBuffer == VK_NULL_HANDLE) return;

    const VkDeviceSize size = sizeof(Particle) * _activeParticles;

    VkBuffer staging = VK_NULL_HANDLE;
    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    createBuffer(ctx.device, ctx.physicalDevice, size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        staging, stagingMem);

    void* data = nullptr;
    if (vkMapMemory(ctx.device, stagingMem, 0, size, 0, &data) != VK_SUCCESS || data == nullptr) {
        vkDestroyBuffer(ctx.device, staging, nullptr);
        vkFreeMemory(ctx.device, stagingMem, nullptr);
        throw std::runtime_error("particleSystem: vkMapMemory failed for staging");
    }
    std::memcpy(data, _particles.data(), static_cast<size_t>(size));
    vkUnmapMemory(ctx.device, stagingMem);

    // Use local copy helper with our stored device/queue/pool
    copyBuffer(_device, _graphicsQueue, _commandPool, staging, _instanceBuffer, size);

    vkDestroyBuffer(ctx.device, staging, nullptr);
    vkFreeMemory(ctx.device, stagingMem, nullptr);
}

void particleSystem::destroy(const RenderContext& ctx)
{
    if (_instanceBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(ctx.device, _instanceBuffer, nullptr);
        _instanceBuffer = VK_NULL_HANDLE;
    }
    if (_instanceMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(ctx.device, _instanceMemory, nullptr);
        _instanceMemory = VK_NULL_HANDLE;
    }
    _mapped = nullptr;
}

void particleSystem::recordDraw(VkCommandBuffer cmd, VkPipeline pipeline, VkBuffer quadVB, VkBuffer quadIB, uint32_t quadIndexCount) const
{
    if (_activeParticles == 0) return;
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Bind quad vertices (binding 0) and per-instance buffer (binding 1)
    std::array<VkBuffer, 2> vertexBuffers = { quadVB, _instanceBuffer };
    const std::span<VkBuffer, 2> vbSpan(vertexBuffers);
    std::array<VkDeviceSize, 2> offsets = { 0, 0 };
    const std::span<VkDeviceSize, 2> offsetSpan(offsets);
    vkCmdBindVertexBuffers(cmd, 0, 2, vbSpan.data(), offsetSpan.data());

    // Use UINT16 to match how indexBuffer was created and bound elsewhere
    vkCmdBindIndexBuffer(cmd, quadIB, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(cmd, quadIndexCount, _activeParticles, 0, 0, 0);
}

void particleSystem::spawnRainArea(const glm::vec3& centerXZ, const glm::vec2& halfSizeXZ, float yTop,
    uint32_t count, float speedMin, float speedMax,
    float lifetimeMin, float lifetimeMax,
    const glm::vec2& windXZ)
{
    if (count == 0 || _maxParticles == 0) return;

    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution<float> rx(-halfSizeXZ.x, halfSizeXZ.x);
    std::uniform_real_distribution<float> rz(-halfSizeXZ.y, halfSizeXZ.y);
    std::uniform_real_distribution<float> spd(speedMin, speedMax);
    std::uniform_real_distribution<float> life(lifetimeMin, lifetimeMax);

    for (uint32_t i = 0; i < count && _particles.size() < _maxParticles; ++i)
    {
        Particle p{};
        p.position = glm::vec3(centerXZ.x + rx(rng), yTop, centerXZ.z + rz(rng));
        const float vy = -spd(rng); // downward
        p.velocity = glm::vec3(windXZ.x, vy, windXZ.y);
        p.maxLifetime = life(rng);
        p.lifetime = p.maxLifetime;
        _particles.push_back(p);
    }
    _activeParticles = static_cast<uint32_t>(_particles.size());
}




