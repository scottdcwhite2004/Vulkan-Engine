#pragma once
#include "GraphicsObject.h"
#include "glm/glm.hpp"
#include <cstdint>
#include <array>

enum class LightType : std::uint32_t
{
    Point = 0,
    Directional = 1,
    Spot = 2
};

// Match GLSL std140 layout and names
struct alignas(16) GPULight
{
    alignas(16) glm::vec3 position;  
    alignas(16) glm::vec3 direction; 
    alignas(16) glm::vec3 color;
    alignas(16) float pad1{ 0.0f };
    alignas(16) float pad2{ 0.0f };
    alignas(16) float pad3{ 0.0f };
    
    alignas(4)  std::uint32_t type;
    alignas(4)  float ambient;
    alignas(4)  float specular;
    alignas(4)  float pad0;

    alignas(4)  float innerCos;
    alignas(4)  float outerCos;
    alignas(4)  float range;
    alignas(4)  float pad4{ 0.0f };

    alignas(4)  float attConst;
    alignas(4)  float attLinear;
    alignas(4)  float attQuadratic;
    alignas(4)  float pad5{ 0.0f };
};

struct alignas(16) LightingUBO
{
    // Move array of GPULight earlier to reduce padding and improve locality
    static constexpr int MaxLights = 8;
    std::array<GPULight, MaxLights> lights;

    alignas(16) glm::vec3 viewPosWorld; float shininess;
    alignas(4)  std::int32_t lightCount;
    alignas(4)  std::uint32_t padA{ 0 };
    alignas(4)  std::uint32_t padB{ 0 };
    alignas(4)  std::uint32_t padC{ 0 };

};

struct GPULightCPU {
    // Group vec3 fields together to avoid interleaved padding and improve locality
    alignas(16) glm::vec3 position;
    alignas(16) glm::vec3 direction;
    alignas(16) glm::vec3 color;

    // Scalars grouped after vector fields
    std::uint32_t type;
    float ambient;
    float specular;

    float innerCos;
    float outerCos;
    float range;

    float attConst;
    float attLinear;
    float attQuadratic;

    // Optional pad to keep size multiple of 16 if needed by upload logic
    float pad0;
};

struct LightingUBOCPU {
    static constexpr int MaxLights = LightingUBO::MaxLights;
    std::array<GPULightCPU, MaxLights> lights;

    alignas(16) glm::vec3 viewPosWorld; float shininess;
    int32_t lightCount; uint32_t padA; uint32_t padB; uint32_t padC;
};


class Light : public GraphicsObject
{
    LightType type{ LightType::Point };

    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    glm::vec3 directionWS{ -0.5f, -1.0f, -0.3f };
    glm::vec3 positionWS{ 0.0f, 2.0f, 2.0f };
    float ambient{ 0.1f };
    float specular{ 0.5f };

    // Reorder vec3 members for improved memory layout


    float range{ 25.0f };
    float attConst{ 1.0f };
    float attLinear{ 0.09f };
    float attQuadratic{ 0.032f };

    float innerAngleDeg{ 20.0f };
    float outerAngleDeg{ 30.0f };

public:
    explicit Light(const glm::vec3& position) : GraphicsObject(position) {}
    Light() = default;
    virtual ~Light();

    void setType(LightType t) { type = t; }
    void setColor(const glm::vec3& c) { color = c; }
    void setAmbient(float a) { ambient = a; }
    void setSpecular(float s) { specular = s; }

    void setPositionWS(const glm::vec3& p) { positionWS = p; }
    void setDirectionWS(const glm::vec3& d) { directionWS = d; }

    void setRange(float r) { range = r; }
    void setAttenuation(float c, float l, float q) { attConst = c; attLinear = l; attQuadratic = q; }

    void setSpotAnglesDeg(float innerDeg, float outerDeg) { innerAngleDeg = innerDeg; outerAngleDeg = outerDeg; }

    GPULightCPU toGPULight() const
    {
        GPULightCPU gpuLight{};
        gpuLight.type = static_cast<std::uint32_t>(type);
        gpuLight.ambient = ambient;
        gpuLight.specular = specular;

        gpuLight.position = positionWS;
        gpuLight.direction = directionWS;
        gpuLight.color = color;

        gpuLight.innerCos = glm::cos(glm::radians(innerAngleDeg));
        gpuLight.outerCos = glm::cos(glm::radians(outerAngleDeg));
        gpuLight.range = range;

        gpuLight.attConst = attConst;
        gpuLight.attLinear = attLinear;
        gpuLight.attQuadratic = attQuadratic;

        return gpuLight;
    }
};