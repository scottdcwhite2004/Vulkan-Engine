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
 //   if (deltaTime < 0.0f)
 //   {
 //       return;
 //   }

 //   if(!_isBurning)
 //   {
	//	timeSinceBurned += deltaTime;
 //       if (timeSinceBurned >= timeToBurn)
 //       {
	//		_isBurning = true;
 //       }
	//}
 //   else
 //   {
	//	burnDuration -= deltaTime;
 //       if (burnDuration <= 0.0f)
 //       {
	//		_isBurning = false;
	//		burnDuration = 10.0f;
	//		timeSinceBurned = 0.0f;
 //       }
 //   }
}