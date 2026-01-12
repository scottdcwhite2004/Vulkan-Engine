#pragma once
#include <string>
#include <Mesh.h>
#include <Material.h>
#include <textureManager.h>
#include <glm/glm.hpp>
#include <IWorldObject.h>

class Rock final : public IWorldObject
{
    static constexpr const char* kModelPath = "models/rock.obj";
public:
    Rock(const glm::vec3& position, textureManager* textureMgr)
        : IWorldObject(position, kModelPath, textureMgr, "rock")
    {
    }

    Rock(const Rock& other) = default;
    Rock& operator=(const Rock& other) = default;
    Rock(Rock&& other) noexcept = default;
    Rock& operator=(Rock&& other) noexcept = default;
    ~Rock() override final;
};