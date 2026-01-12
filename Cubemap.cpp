#include "Cubemap.h"
#include <cstring>
#include <array>
#include <string>


namespace {

	void createCubemapImage(const Image& img, uint32_t size, VkFormat format, VkImage& image)
	{
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { size, size, 1 };
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 6;
        imageInfo.format = format;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        const VkDevice device = img.device();
        if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("Cubemap: vkCreateImage failed");
        }
	}
}

void Cubemap::create(uint32_t faceSize, VkFormat format, const std::array<const void*, 6>& facePixelData, const VkSamplerCreateInfo& samplerInfo)
{
    if (_device == VK_NULL_HANDLE) {
        throw std::runtime_error("Cubemap: device not set (use constructor with device/physical/commandPool/queue).");
    }
    if (faceSize == 0) {
        throw std::runtime_error("Cubemap: faceSize must be > 0.");
    }
    _size = faceSize;
    _format = format;

    createGpuImage();
    uploadFaces(facePixelData);
    createView();
    createSampler(samplerInfo);
}

void Cubemap::destroy() {
    if (_sampler) { vkDestroySampler(_device, _sampler, nullptr); _sampler = VK_NULL_HANDLE; }
    if (_imageView) { vkDestroyImageView(_device, _imageView, nullptr); _imageView = VK_NULL_HANDLE; }
    if (_imageHandle) {
        vkDestroyImage(_device, _imageHandle, nullptr);
        _imageHandle = VK_NULL_HANDLE;
    }
    if (_imageMemory) {
        vkFreeMemory(_device, _imageMemory, nullptr);
        _imageMemory = VK_NULL_HANDLE;
    }
}

void Cubemap::createGpuImage() {
    // Create image and allocate memory via Image helpers (generalized path here)
    // Manual creation since Image::createImage does not take arrayLayers/flags:
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { _size, _size, 1 };
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 6;
    imageInfo.format = _format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(_device, &imageInfo, nullptr, &_imageHandle) != VK_SUCCESS) {
        throw std::runtime_error("Cubemap: failed to create image");
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(_device, _imageHandle, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = _image.findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(_device, &allocInfo, nullptr, &_imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("Cubemap: failed to allocate image memory");
    }
    vkBindImageMemory(_device, _imageHandle, _imageMemory, 0);
}

Cubemap::Staging Cubemap::createAndFillStaging(const void* pixels, VkDeviceSize byteSize) {
    // Create a staging buffer using Source.cpp’s pattern; we re-implement here for encapsulation.
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = byteSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    Staging s{};
    if (vkCreateBuffer(_device, &bufferInfo, nullptr, &s.buffer) != VK_SUCCESS) {
        throw std::runtime_error("Cubemap: failed to create staging buffer");
    }

    VkMemoryRequirements memReq{};
    vkGetBufferMemoryRequirements(_device, s.buffer, &memReq);

    VkMemoryAllocateInfo alloc{};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = memReq.size;
    alloc.memoryTypeIndex = _image.findMemoryType(memReq.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    if (vkAllocateMemory(_device, &alloc, nullptr, &s.memory) != VK_SUCCESS) {
        throw std::runtime_error("Cubemap: failed to allocate staging memory");
    }

    vkBindBufferMemory(_device, s.buffer, s.memory, 0);

    void* dst = nullptr;
    if (vkMapMemory(_device, s.memory, 0, byteSize, 0, &dst) != VK_SUCCESS || dst == nullptr) {
        throw std::runtime_error("Cubemap: vkMapMemory failed");
    }
    std::memcpy(dst, pixels, static_cast<size_t>(byteSize));
    vkUnmapMemory(_device, s.memory);

    return s;
}

void Cubemap::uploadFaces(const std::array<const void*, 6>& facePixelData) {
    // Transition to TRANSFER_DST for all layers
    {
        const VkCommandBuffer cmd = _image.beginSingleTimeCommands();
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _imageHandle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        _image.endSingleTimeCommands(cmd);
    }

    // Copy each face (array layer 0..5)
    const VkDeviceSize bytesPerFace = static_cast<VkDeviceSize>(_size) * _size * 4; // RGBA8
    for (uint32_t face = 0; face < 6; ++face) {
        const void* const src = facePixelData[face];
        if (!src) { throw std::runtime_error("Cubemap: missing face data at index " + std::to_string(face)); }

        Staging s = createAndFillStaging(src, bytesPerFace);

        const VkCommandBuffer cmd = _image.beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = face;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { _size, _size, 1 };

        vkCmdCopyBufferToImage(cmd, s.buffer, _imageHandle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        _image.endSingleTimeCommands(cmd);

        vkDestroyBuffer(_device, s.buffer, nullptr);
        vkFreeMemory(_device, s.memory, nullptr);
    }

    // Transition to SHADER_READ_ONLY for sampling
    {
        const VkCommandBuffer cmd = _image.beginSingleTimeCommands();
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = _imageHandle;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &barrier);

        _image.endSingleTimeCommands(cmd);
    }
}

void Cubemap::createView() {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = _imageHandle;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    viewInfo.format = _format;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 6;

    if (vkCreateImageView(_device, &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
        throw std::runtime_error("Cubemap: failed to create image view");
    }
}

void Cubemap::createSampler(const VkSamplerCreateInfo& samplerInfo) {
    if (vkCreateSampler(_device, &samplerInfo, nullptr, &_sampler) != VK_SUCCESS) {
        throw std::runtime_error("Cubemap: failed to create sampler");
    }
}
