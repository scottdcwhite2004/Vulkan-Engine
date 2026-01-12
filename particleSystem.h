#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include "Particle.h"
#include "RenderContext.h"
#include <array>
#include <span>

class particleSystem final
{
    glm::vec3 _origin{};
    std::vector<Particle> _particles;

    VkBuffer _instanceBuffer{ VK_NULL_HANDLE };
    VkDeviceMemory _instanceMemory{ VK_NULL_HANDLE };
    void* _mapped{ nullptr };

    VkDevice _device{ VK_NULL_HANDLE };
    VkPhysicalDevice _physicalDevice{ VK_NULL_HANDLE };
    VkCommandPool _commandPool{ VK_NULL_HANDLE };
    VkQueue _graphicsQueue{ VK_NULL_HANDLE };

    uint32_t _maxParticles{};
    uint32_t _activeParticles{};

    void ensureGPUBuffer(const RenderContext& ctx);

public:
    particleSystem() = default;
    particleSystem(const particleSystem&) = delete;
    particleSystem& operator=(const particleSystem&) = delete;
    particleSystem(particleSystem&&) noexcept = default;
    particleSystem& operator=(particleSystem&&) noexcept = default;

    particleSystem(const glm::vec3& origin, uint32_t maxParticles)
        : _origin(origin), _particles(), _instanceBuffer(VK_NULL_HANDLE), _instanceMemory(VK_NULL_HANDLE), _mapped(nullptr),
        _device(VK_NULL_HANDLE), _physicalDevice(VK_NULL_HANDLE), _commandPool(VK_NULL_HANDLE), _graphicsQueue(VK_NULL_HANDLE),
        _maxParticles(maxParticles), _activeParticles(0)
    {
        _particles.reserve(maxParticles);
    }

    ~particleSystem() = default;

    void setOrigin(const glm::vec3& origin) { _origin = origin; }
    void spawnBurst(uint32_t count, float speedMin, float speedMax);
    void update(float deltaTime);

    // New: Spawn vertical rain in an XZ area at yTop with downward velocity and optional wind
    void spawnRainArea(const glm::vec3& centerXZ, const glm::vec2& halfSizeXZ, float yTop,
        uint32_t count, float speedMin, float speedMax,
        float lifetimeMin, float lifetimeMax,
        const glm::vec2& windXZ = glm::vec2(0.0f, 0.0f));

    inline void create(const RenderContext& ctx) { ensureGPUBuffer(ctx); }
    inline void uploadDescriptors(const RenderContext& ctx) { uploadInstances(ctx); }

    void destroy(const RenderContext& ctx);
    void recordDraw(VkCommandBuffer cmd, VkPipeline pipeline, VkBuffer quadVB, VkBuffer quadIB, uint32_t quadIndexCount) const;
    void uploadInstances(const RenderContext& ctx);
    VkBuffer instanceBuffer() const { return _instanceBuffer; }
    VkDeviceMemory instanceMemory() const { return _instanceMemory; }
    uint32_t aliveCount() const { return _activeParticles; }
};