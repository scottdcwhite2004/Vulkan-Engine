#include "Candle.h"

Candle::~Candle() = default;

void Candle::initializeFlame(const RenderContext& ctx)
{
	_flameParticles = particleSystem(position() + glm::vec3(0.0f, 1.0f, 0.0f), 1000);
	_flameParticles.create(ctx);
}

void Candle::update(float& deltaTime)
{
	if (deltaTime < 0.0f)
	{
		return;
	}
	// Spawn flame particles continuously
	_flameParticles.spawnBurst(5, 0.5f, 1.5f);
	_flameParticles.update(deltaTime);
}
