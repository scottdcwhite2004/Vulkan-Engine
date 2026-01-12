#include "GlobeScene.h"
#include <stdexcept>

GlobeScene::~GlobeScene()
{
    for (auto obj : _objects)
    {
        delete obj;
    }
}

GlobeScene::GlobeScene(GlobeScene&& other) noexcept
    : _objects(std::move(other._objects)),
    _textureMgr(other._textureMgr),
    _cameraMgr(other._cameraMgr),
    _configLoader(std::move(other._configLoader)),
    _isRaining(other._isRaining),
    timeSinceRain(other.timeSinceRain),
    _rainInterval(other._rainInterval),
    _rainDuration(other._rainDuration),
    _rainParticleSystem(other._rainParticleSystem),
    _rainAreaSize(other._rainAreaSize),
    _rainParticleSpeed(other._rainParticleSpeed),
    _rainParticleSize(other._rainParticleSize),
    _dayNightCycleDuration(other._dayNightCycleDuration),
    _timeOfDay(other._timeOfDay),
    _postProcessObjects(std::move(other._postProcessObjects)),
    _sunNoRainToIgnite(other._sunNoRainToIgnite)
{
    other._textureMgr = nullptr;
    other._cameraMgr = nullptr;
    other._rainParticleSystem = nullptr;
}

GlobeScene& GlobeScene::operator=(GlobeScene&& other) noexcept
{
    if (this != &other)
    {
        for (auto obj : _objects)
        {
            delete obj;
        }
        delete _rainParticleSystem;
        _objects = std::move(other._objects);
        _textureMgr = other._textureMgr;
        _cameraMgr = other._cameraMgr;
        _configLoader = std::move(other._configLoader);
        _isRaining = other._isRaining;
        timeSinceRain = other.timeSinceRain;
        _rainInterval = other._rainInterval;
        _rainDuration = other._rainDuration;
        _rainParticleSystem = other._rainParticleSystem;
        _rainAreaSize = other._rainAreaSize;
        _rainParticleSpeed = other._rainParticleSpeed;
        _rainParticleSize = other._rainParticleSize;
        _dayNightCycleDuration = other._dayNightCycleDuration;
        _timeOfDay = other._timeOfDay;
        _postProcessObjects = std::move(other._postProcessObjects);
        _sunNoRainToIgnite = other._sunNoRainToIgnite;

        other._textureMgr = nullptr;
        other._cameraMgr = nullptr;
        other._rainParticleSystem = nullptr;
    }
    return *this;
}

void GlobeScene::addObject(IWorldObject* object)
{
    _objects.push_back(object);
}

void GlobeScene::drawScene(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline, uint32_t currentFrame)
{
	_postProcessObjects.clear();
    for (auto obj : _objects)
    {
    //    if (static_cast<Cactus*>(obj))
    //    {
    //        if (static_cast<Cactus*>(obj)->isBurning())
    //        {
    //            _postProcessObjects.insert(obj);
    //        }
    //        else
    //        {
				//obj->draw(commandBuffer, graphicsPipeline, pipelineLayout, currentFrame);
    //        }
    //    }
    //    else
    //    {
    //        obj->draw(commandBuffer, graphicsPipeline, pipelineLayout, currentFrame);
    //    }
		obj->draw(commandBuffer, graphicsPipeline, pipelineLayout, currentFrame);

    }
}

void GlobeScene::updateScene(float deltaTime)
{
    _timeOfDay += deltaTime;
    if (_timeOfDay >= _dayNightCycleDuration) _timeOfDay -= _dayNightCycleDuration;
    for (auto obj : _objects)
    {
        obj->update(deltaTime);
	}

}

void GlobeScene::drawPostProcessables(VkCommandBuffer& commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline graphicsPipeline, uint32_t currentFrame)
{
    for (auto* obj : _postProcessObjects)
    {
        obj->draw(commandBuffer, graphicsPipeline, pipelineLayout, currentFrame);
    }
}

void GlobeScene::destroyScene(const RenderContext& ctx)
{
    for (auto obj : _objects)
    {
        obj->destroy(ctx);
    }
    if (_rainParticleSystem)
    {
        _rainParticleSystem->destroy(ctx);
    }
    _postProcessObjects.clear();
}

void GlobeScene::uploadScene(const RenderContext& ctx, uint32_t framesInFlight,
    VkImageView textureImageView, VkSampler textureSampler,
    const std::vector<VkDescriptorBufferInfo>& lightingBufferInfos)
{
    for (auto obj : _objects)
    {
        obj->upload(ctx, framesInFlight, textureImageView, textureSampler, lightingBufferInfos);
    }
    if (_rainParticleSystem)
    {
        _rainParticleSystem->uploadDescriptors(ctx);
    }
}

void GlobeScene::updateSceneUniformBuffers(uint32_t frameIndex,
    const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj)
{
    for (auto obj : _objects)
    {
        obj->updateUniformBuffer(frameIndex, model, view, proj);
    }
}

void GlobeScene::initializeScene()
{
    _rainParticleSystem = new particleSystem(glm::vec3(0.0f, 10.0f, 0.0f), 5000);
}

void GlobeScene::loadScene()
{
	loadSceneFromFile("scene_objects.csv");
}

void GlobeScene::setObjectPostProcess(IWorldObject* obj, bool enable)
{
    if (!obj) return;
    obj->setUsesPostProcess(enable);
    if (enable)
        _postProcessObjects.insert(obj);
    else
        _postProcessObjects.erase(obj);
}

std::vector<Light> GlobeScene::getCandleLights() const
{
    std::vector<Light> candleLights;
    for (auto* obj : _objects)
    {
        if (auto* candle = dynamic_cast<Candle*>(obj))
        {
            candleLights.push_back(candle->getLight());
        }
    }
    return candleLights;
}

void GlobeScene::loadSceneFromFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Could not open scene file: " << filename << std::endl;
        return;
    }

    std::string line;
    bool isFirstLine = true;
    while (std::getline(file, line))
    {
        // Skip header
        if (isFirstLine) {
            isFirstLine = false;
            continue;
        }

        std::istringstream iss(line);
        std::string type, sx, sy, sz;

        if (!std::getline(iss, type, ',')) {
            std::cerr << "Failed to parse type in line: " << line << std::endl;
            continue;
        }
        if (!std::getline(iss, sx, ',')) {
            std::cerr << "Failed to parse x in line: " << line << std::endl;
            continue;
        }
        if (!std::getline(iss, sy, ',')) {
            std::cerr << "Failed to parse y in line: " << line << std::endl;
            continue;
        }
        if (!std::getline(iss, sz, ',')) {
            std::cerr << "Failed to parse z in line: " << line << std::endl;
            continue;
        }

        float x = std::stof(sx);
        float y = std::stof(sy);
        float z = std::stof(sz);

        std::cout << "Parsed: " << type << " at (" << x << "," << y << "," << z << ")" << std::endl;

        IWorldObject* obj = nullptr;
        if (type == "Cactus") obj = new Cactus(glm::vec3(x, y, z), _textureMgr);
        else if (type == "Rock") obj = new Rock(glm::vec3(x, y, z), _textureMgr);
        else if (type == "Camel") obj = new Camel(glm::vec3(x, y, z), _textureMgr);
        else if (type == "Candle") obj = new Candle(glm::vec3(x, y, z), _textureMgr);

        if (obj) addObject(obj);
        else std::cerr << "Unknown object type: " << type << std::endl;
    }
    std::cout << "Loaded " << _objects.size() << " objects from file." << std::endl;
}