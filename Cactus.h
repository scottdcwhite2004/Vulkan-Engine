#pragma once
#include <string>
#include <Mesh.h>
#include <Material.h>
#include <textureManager.h>
#include <glm/glm.hpp>
#include <IWorldObject.h>

class Cactus final : public IWorldObject
{
    static constexpr const char* kModelPath = "models/cactus.obj";

    const float _maxGrowthHeight = 20.0f;
    float _growthRate = 0.02f;
    float _currentHeight = 0.1f;
    float timeSinceRain = 0.0f;
    float timeToBurn = 3.0f;
	float timeSinceBurned = 0.0f;
	float burnDuration = 10.0f;

    bool _isBurning = false;
    bool _isRaining = false;
    bool _isSunny = true;

public:
    Cactus(const glm::vec3& position, textureManager* textureMgr)
        : IWorldObject(position, kModelPath, textureMgr, "cactus")
    {
    }

    Cactus(const Cactus& other) = default;
    Cactus& operator=(const Cactus& other) = default;
    Cactus(Cactus&& other) noexcept = default;
    Cactus& operator=(Cactus&& other) noexcept = default;
    ~Cactus() override final;

    void grow();
    void update(float& deltaTime) override final;

    bool isBurning() const { return _isBurning; }

    // NEW: when toggled, also tag for post-process
    void setBurning(bool burning)
    {
        _isBurning = burning;
        setUsesPostProcess(burning);
    }
};