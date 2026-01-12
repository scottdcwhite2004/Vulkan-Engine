#pragma once
#include "glm/glm.hpp"
#include "vulkan/vulkan.h"
#include "Texture.h"
#include <string>


class Material final
{
	glm::vec4 _albedoColour;
		float _metallic;
		float _roughness;
		Texture* _texture = nullptr;

public:
	Material(const glm::vec4& albedoColour, float metallic, float roughness, Texture* texture) : _albedoColour(albedoColour), _metallic(metallic), _roughness(roughness), _texture(texture) {}
	Material() = default;
	~Material() = default;

	Material(const Material&) = default;
	Material& operator=(const Material&) = default;

	void apply() const;
	VkSampler getTextureSampler() const { return (_texture != nullptr) ? _texture->getTextureSampler() : VK_NULL_HANDLE; }
	VkImageView getTextureImageView() const { return (_texture != nullptr) ? _texture->getTextureImageView() : VK_NULL_HANDLE; }
	void setTexture(Texture* texture) { _texture = texture; }
};

