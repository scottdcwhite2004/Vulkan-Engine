#include "IWorldObject.h"

IWorldObject::~IWorldObject() = default;

void IWorldObject::draw(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout layout, uint32_t currentFrame)
{
    _mesh.draw(cmd, pipeline, layout, currentFrame);
}

void IWorldObject::upload(const RenderContext& ctx, uint32_t framesInFlight,
    VkImageView textureImageView, VkSampler textureSampler,
    const std::vector<VkDescriptorBufferInfo>& lightingBufferInfos)
{
    _mesh.upload(ctx, framesInFlight, _texture->getTextureImageView(), _texture->getTextureSampler(), lightingBufferInfos);
}

void IWorldObject::destroy(const RenderContext& ctx)
{
    _mesh.destroy(ctx);
}

void IWorldObject::updateUniformBuffer(uint32_t frameIndex,
    const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj) const
{
    _mesh.updateUniformBuffer(frameIndex, model, view, proj);
}

void IWorldObject::update(float& /*deltaTime*/)
{
    // Default no-op for base type
}