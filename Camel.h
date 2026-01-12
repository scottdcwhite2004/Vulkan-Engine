#pragma once
#include <string>
#include <Mesh.h>
#include <Material.h>
#include <textureManager.h>
#include <glm/glm.hpp>
#include <IWorldObject.h>

class Camel final : public IWorldObject
{
    static constexpr const char* kModelPath = "models/camel.obj";
public:
    Camel(const glm::vec3& position, textureManager* textureMgr)
        : IWorldObject(position, kModelPath, textureMgr, "camel")
    {
    }

    Camel(const Camel& other) = default;
    Camel& operator=(const Camel& other) = default;
    Camel(Camel&& other) noexcept = default;
    Camel& operator=(Camel&& other) noexcept = default;
    ~Camel() override final;
};