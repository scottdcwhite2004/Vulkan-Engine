#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <unordered_set>
#include "IWorldObject.h"
#include "CameraManager.h"
#include "configLoader.h"
#include "CameraManager.h"
#include "textureManager.h"
#include "particleSystem.h"
#include "Cactus.h"
#include "Rock.h"
#include "Candle.h"
#include "Camel.h"


class GlobeScene final
{
    std::vector<IWorldObject*> _objects;
    textureManager* _textureMgr{ nullptr };
    CameraManager* _cameraMgr{ nullptr };

    const std::string _sceneConfigPath{ "configs/sceneConfig.json" };
    configLoader _configLoader{};
    bool _isRaining{ false };
    float timeSinceRain{};
    float _rainInterval{ 70.0f }; // seconds between rain starts
    float _rainDuration{ 20.0f };  // seconds rain lasts

    particleSystem* _rainParticleSystem{ nullptr };
    float _rainAreaSize{ 50.0f }; // size of the square area where rain occurs
    float _rainParticleSpeed{ 5.0f }; // speed of rain particles
    float _rainParticleSize{ 0.1f }; // size of rain particles

    float _dayNightCycleDuration{ 60.0f }; // total duration of day-night cycle in seconds
    float _timeOfDay{ 0.0f }; // current time in the day-night cycle
    const float _dayBegin{ 0.0f };
    const float _nightBegin{ 30.0f }; // time when night starts

    // NEW: any objects to be affected by the post-process (mask rendering)
    std::unordered_set<IWorldObject*> _postProcessObjects;

    // NEW: threshold for ignition: sunny with no rain for this many seconds
    float _sunNoRainToIgnite{ 0.0f };

public:
    GlobeScene() = default;
    ~GlobeScene();
    GlobeScene(textureManager& textureMgr, CameraManager& cameraMgr) : _textureMgr(&textureMgr), _cameraMgr(&cameraMgr), _configLoader(configLoader{}) {}
    // Non-copyable: owning raw pointers and GPU resources cannot be copied safely
    GlobeScene(const GlobeScene&) = delete;
    GlobeScene& operator=(const GlobeScene&) = delete;
    // Movable: transfer ownership
    GlobeScene(GlobeScene&& other) noexcept;
    GlobeScene& operator=(GlobeScene&& other) noexcept;

    void addObject(IWorldObject* object);
    void drawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline, uint32_t currentFrame);
    void uploadScene(const RenderContext& ctx, uint32_t framesInFlight,
        VkImageView textureImageView, VkSampler textureSampler,
        const std::vector<VkDescriptorBufferInfo>& lightingBufferInfos);
    void updateSceneUniformBuffers(uint32_t frameIndex,
        const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj);

    // This will be used in a mask render pass: only draw post-process targets
    void drawPostProcessables(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline, uint32_t currentFrame);
    void destroyScene(const RenderContext& ctx);
    const std::vector<IWorldObject*>& getObjects() const { return _objects; }
    CameraManager* getCameraManager() const { return _cameraMgr; }
    void updateScene(float deltaTime);
    void initializeScene();
    void loadScene();
    float getTimeOfDay() const { return _timeOfDay; }
    float getDayNightCycleDuration() const { return _dayNightCycleDuration; }
    void setRainParticleSystem(particleSystem* rainParticleSystem) { _rainParticleSystem = rainParticleSystem; }
    bool isRaining() const { return _isRaining; }
    std::vector<Light> getCandleLights() const;
	void loadSceneFromFile(const std::string& filename);

    // NEW: expose current set for debugging if needed
    const std::unordered_set<IWorldObject*>& getPostProcessObjects() const { return _postProcessObjects; }

    // NEW: toggle membership in post-process set
    void setObjectPostProcess(IWorldObject* obj, bool enable);

    void reset();
};