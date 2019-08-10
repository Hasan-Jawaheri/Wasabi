#pragma once

#include "Wasabi/Core/WCommon.h"
#include "Wasabi/Memory/WVulkanMemoryManager.h"

struct WBufferedImageProperties {
	VkFormat format;
	W_MEMORY_STORAGE memory;
	VkImageUsageFlags usage;
	VkSampleCountFlagBits sampleCount;
	VkImageType type;
	uint arraySize;
	uint mipLevels;

	WBufferedImageProperties() {}
	WBufferedImageProperties(
		VkFormat _format,
		W_MEMORY_STORAGE _memory = W_MEMORY_DEVICE_LOCAL,
		VkImageUsageFlags _usage = VK_IMAGE_USAGE_SAMPLED_BIT,
		uint _arraySize = 1,
		uint _mipLevels = 1,
		VkImageType _type = VK_IMAGE_TYPE_2D,
		VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT
	) : format(_format), memory(_memory), usage(_usage), sampleCount(_sampleCount), type(_type), arraySize(_arraySize), mipLevels(_mipLevels) {}
};

class WBufferedImage {
public:
	WBufferedImage();

	VkResult Create(class Wasabi* app, uint numBuffers, uint width, uint height, uint depth, WBufferedImageProperties properties, void* pixels = nullptr);
	void Destroy(class Wasabi* app);

	VkResult Map(class Wasabi* app, uint bufferIndex, void** pixels, W_MAP_FLAGS flags);
	void Unmap(class Wasabi* app, uint bufferIndex);

	VkImageView GetView(class Wasabi* app, uint bufferIndex) const;
	VkImageLayout GetLayout(uint bufferIndex) const;
	void TransitionLayoutTo(VkCommandBuffer cmdBuf, VkImageLayout newLayout, uint bufferIndex);

	bool Valid() const;
	size_t GetMemorySize() const;
	uint GetWidth() const;
	uint GetHeight() const;
	uint GetDepth() const;
	uint GetArraySize() const;

private:
	VkImageAspectFlags m_aspect;
	WBufferedImageProperties m_properties;
	size_t m_bufferSize;
	uint m_width;
	uint m_height;
	uint m_depth;
	W_MAP_FLAGS m_lastMapFlags;

	void* m_readOnlyMemory;
	std::vector<WVulkanImage> m_images;
	std::vector<WVulkanBuffer> m_stagingBuffers;
	std::vector<VkImageLayout> m_layouts;

	VkResult CopyStagingToImage(class Wasabi* app, WVulkanBuffer& buffer, WVulkanImage& image, VkImageLayout& initialLayout);
};