#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstdint>
#include <stdexcept>
#include "Image.h"
class Cubemap final
{

    struct Staging
    {
        VkBuffer buffer{ VK_NULL_HANDLE };
		VkDeviceMemory memory{ VK_NULL_HANDLE };
        Staging() = default;

        Staging(const Staging& other) = default;

        Staging& operator=(const Staging& other) = default;


        Staging(Staging&& other) = default;
        
        Staging& operator=(Staging&& other) = default;
    };

    // Largest composite object first (class with dynamic resources)
    Image _image{};

    // Handles and lightweight PODs next
    VkDevice _device;
    VkImage _imageHandle = VK_NULL_HANDLE;
    VkDeviceMemory _imageMemory = VK_NULL_HANDLE;
    VkImageView _imageView = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

    uint32_t _size = 0;
    VkFormat _format = VK_FORMAT_UNDEFINED;

    void createGpuImage();
	void uploadFaces(const std::array<const void*, 6>& facePixelData);
    void createView();
	void createSampler(const VkSamplerCreateInfo& samplerInfo);

    Staging createAndFillStaging(const void* pixels, VkDeviceSize byteSize);

public:
	Cubemap() = default;

    Cubemap(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue)
        : _image(device, physicalDevice, commandPool, graphicsQueue),
        _device(device) {
    }

    Cubemap(const Cubemap& other)
        : _image(), // keep default-constructed; no resource copy
        _device(other._device),
        _imageHandle(VK_NULL_HANDLE),
        _imageMemory(VK_NULL_HANDLE),
        _imageView(VK_NULL_HANDLE),
        _sampler(VK_NULL_HANDLE),
        _size(0),
        _format(VK_FORMAT_UNDEFINED)
    {
    }

    Cubemap& operator=(const Cubemap& other)
    {
        if (this == &other) {
            return *this;
        }
        // Intentionally no copying of Vulkan resources; preserves current semantics.
        // Keep device consistent with source object.
        _device = other._device;
        _imageHandle = VK_NULL_HANDLE;
        _imageMemory = VK_NULL_HANDLE;
        _imageView = VK_NULL_HANDLE;
        _sampler = VK_NULL_HANDLE;
        _size = 0;
        _format = VK_FORMAT_UNDEFINED;
        _image = Image{}; // reset to default; no resource copy
        return *this;
    }

    // Movable
    Cubemap(Cubemap&& other)
        : _image(std::move(other._image)),
        _device(other._device),
        _imageHandle(other._imageHandle),
        _imageMemory(other._imageMemory),
        _imageView(other._imageView),
        _sampler(other._sampler),
        _size(other._size),
        _format(other._format) {
        other._imageHandle = VK_NULL_HANDLE;
        other._imageMemory = VK_NULL_HANDLE;
        other._imageView = VK_NULL_HANDLE;
        other._sampler = VK_NULL_HANDLE;
        other._size = 0;
        other._format = VK_FORMAT_UNDEFINED;
    }

    Cubemap& operator=(Cubemap&& other) {
        if (this != &other) {
            destroy();

            _image = std::move(other._image);
            _device = other._device;
            _imageHandle = other._imageHandle;
            _imageMemory = other._imageMemory;
            _imageView = other._imageView;
            _sampler = other._sampler;
            _size = other._size;
            _format = other._format;

            other._imageHandle = VK_NULL_HANDLE;
            other._imageMemory = VK_NULL_HANDLE;
            other._imageView = VK_NULL_HANDLE;
            other._sampler = VK_NULL_HANDLE;
            other._size = 0;
            other._format = VK_FORMAT_UNDEFINED;
        }
        return *this;
    }

	void create(uint32_t faceSize, VkFormat format, const std::array<const void*, 6>& facePixelData, const VkSamplerCreateInfo& samplerInfo = DefaultSamplerCreateInfo());

	void destroy();

	VkImage image() const { return _imageHandle; }
	VkImageView view() const { return _imageView; }
	VkSampler sampler() const { return _sampler; }
	uint32_t size() const { return _size; }
	VkFormat format() const { return _format; }

    static VkSamplerCreateInfo DefaultSamplerCreateInfo() {
        VkSamplerCreateInfo si{};
        si.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        si.magFilter = VK_FILTER_LINEAR;
        si.minFilter = VK_FILTER_LINEAR;
        si.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        si.anisotropyEnable = VK_FALSE; // skyboxes generally don’t need anisotropy
        si.maxAnisotropy = 1.0f;
        si.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        si.unnormalizedCoordinates = VK_FALSE;
        si.compareEnable = VK_FALSE;
        si.compareOp = VK_COMPARE_OP_ALWAYS;
        si.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        si.mipLodBias = 0.0f;
        si.minLod = 0.0f;
        si.maxLod = 0.0f;
        return si;
    }
};

