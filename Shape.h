#pragma once
#include "GraphicsObject.h"
#include "vulkan/vulkan.h"
#include "Material.h"
#include <vector>
#include "Vertex.h"
#include "RenderContext.h"
#include "Shape.h"
#include "ObjLoader.h"
#include "glm/glm.hpp"
#include <string>


class Shape :
    public GraphicsObject
{
	
	std::vector<Vertex> _vertices = {};
	std::vector<uint16_t> _indices = {};
	std::vector<VkBuffer> _uniformBuffers;
	std::vector<VkDeviceMemory> _uniformBuffersMemory;
	std::vector<void*> _uniformBuffersMapped;
	std::vector<VkDescriptorSet> _descriptorSets;
	
	Material _material;
	VkBuffer _vertexBuffer{ VK_NULL_HANDLE };
	VkBuffer _indexBuffer{ VK_NULL_HANDLE };
	VkDeviceMemory _vertexBufferMemory{ VK_NULL_HANDLE };
	VkDeviceMemory _indexBufferMemory{ VK_NULL_HANDLE };



    public:
		Shape(const glm::vec3& position, const Material& material) : GraphicsObject(position), _material(material) {};
		Shape() = default;
		~Shape();

		Shape(const Shape&);
		Shape& operator=(const Shape&);

		Shape(Shape&&);

		Shape& operator=(Shape&&);


		inline void applyMaterial() const { _material.apply(); }
		virtual void create() = 0;
		void draw(VkCommandBuffer cmd, VkPipeline pipeline, VkPipelineLayout layout,
			uint32_t currentFrame);
		virtual void move() = 0;
		void upload(const RenderContext& ctx, uint32_t framesInFlight, VkImageView textureImageView, VkSampler textureSampler, const std::vector<VkDescriptorBufferInfo>& lightinBufferInfos);
		void destroy(const RenderContext& ctx);
		void updateUniformBuffer(uint32_t frameIndex, const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj) const;
		void setVertices(const std::vector<Vertex>& vertices) { _vertices = vertices; };
		void setIndices(const std::vector<uint16_t>& indices) { _indices = indices; };
		std::vector<Vertex> getVertices() const { return _vertices; };
		std::vector<uint16_t> getIndices() const { return _indices; };
		const Material getMaterial() const {
			return _material;
		}

};

