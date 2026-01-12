#pragma once
#include <string>
#include <Mesh.h>
#include <Material.h>
#include <textureManager.h>
#include <glm/glm.hpp>
#include <IWorldObject.h>
#include <Light.h>
#include <RenderContext.h>
#include <ParticleSystem.h>

class Candle final : public IWorldObject
{
    static constexpr const char* kModelPath = "models/candle.obj";
	particleSystem _flameParticles{};
    Light _light{};

public:
    Candle(const glm::vec3& position, textureManager* textureMgr)
        : IWorldObject(position, kModelPath, textureMgr, "candle")
    {
        // Set up the point light for the candle flame
        _light.setType(LightType::Point);
        _light.setColor(glm::vec3(1.0f, 0.8f, 0.5f)); // warm flame color
        _light.setAmbient(0.001f);
        _light.setSpecular(0.1f);
        _light.setRange(3.0f); // small radius for a candle
        _light.setAttenuation(1.0f, 0.22f, 0.20f); // tweak for candle falloff

        // Set the light's position to the flame's world position
        glm::vec3 flameOffset(0.0f, 0.15f, 0.0f); // adjust as needed for your model
        _light.setPositionWS(position + flameOffset);
    }

    Candle(const Candle& other) = default;
    Candle& operator=(const Candle& other) = default;
    Candle(Candle&& other) noexcept = default;
    Candle& operator=(Candle&& other) noexcept = default;
    ~Candle() override final;
	void initializeFlame(const RenderContext& ctx);
	void update(float& deltaTime) override final;
    void drawFlame(VkCommandBuffer cmd, VkPipeline pipeline, VkBuffer quadVB, VkBuffer quadIB, uint32_t quadIndexCount) const
    {
        _flameParticles.recordDraw(cmd, pipeline, quadVB, quadIB, quadIndexCount);
	}
	const Light& getLight() const { return _light; }
};