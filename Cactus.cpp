#include "Cactus.h"

Cactus::~Cactus() = default;

void Cactus::grow()
{
    if (_currentHeight < _maxGrowthHeight)
    {
        _currentHeight += _growthRate;
        if (_currentHeight > _maxGrowthHeight)
        {
            _currentHeight = _maxGrowthHeight;
        }
    }
}

void Cactus::update(float& deltaTime)
{
    if (deltaTime < 0.0f)
    {
        return;
    }

    if (_isBurning)
    {
        // Simple burn-down timer; when done, reset and shrink cactus.
        timeToBurn -= deltaTime;
        if (timeToBurn <= 0.0f)
        {
            setBurning(false);
            timeToBurn = 10.0f;
            _currentHeight = 0.0f;
        }
        return;
    }

    // Simple growth model based on weather flags
    float growthFactor = 1.0f;
    if (_isRaining)
    {
        growthFactor = 2.0f; // grows faster in rain
        timeSinceRain = 0.0f;
    }
    else if (!_isSunny)
    {
        growthFactor = 0.5f; // grows slower when not sunny
        timeSinceRain += deltaTime;
    }
    else
    {
        timeSinceRain += deltaTime;
    }

    _currentHeight += _growthRate * growthFactor * deltaTime;
    if (_currentHeight > _maxGrowthHeight)
    {
        _currentHeight = _maxGrowthHeight;
    }


}