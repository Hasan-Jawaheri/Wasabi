#pragma once

#include "../Core/WCommon.h"
#include "WVulkanMemoryManager.h"

struct WBufferedImageProperties {
	VkFormat format;
	W_MEMORY_STORAGE memory;
	VkImageUsageFlags usage;
	VkSampleCountFlagBits sampleCount;
	uint arraySize;
	uint mipLevels;

	WBufferedImageProperties(
		VkFormat fmt,
		W_MEMORY_STORAGE mem = W_MEMORY_DEVICE_LOCAL,
		VkImageUsageFlags usg = VK_IMAGE_USAGE_SAMPLED_BIT,
		uint arrSize = 1,
		uint mips = 1,
		VkSampleCountFlagBits sampleCount = VK_SAMPLE_COUNT_1_BIT
	) : format(fmt), memory(mem), usage(usg), arraySize(arrSize), mipLevels(mips), sampleCount(sampleCount) {}
};

class WBufferedImage2D {
public:
	WBufferedImage2D();

	VkResult Create(class Wasabi* app, uint numBuffers, uint width, uint height, WBufferedImageProperties properties, void* pixels = nullptr);
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

private:
	VkImageAspectFlags m_aspect;
	VkImageUsageFlags m_usage;
	size_t m_bufferSize;
	uint m_width;
	uint m_height;
	W_MAP_FLAGS m_lastMapFlags;

	void* m_readOnlyMemory;
	std::vector<WVulkanImage> m_images;
	std::vector<WVulkanBuffer> m_stagingBuffers;
	std::vector<VkImageLayout> m_layouts;

	VkResult CopyStagingToImage(class Wasabi* app, WVulkanBuffer& buffer, WVulkanImage& image, VkImageLayout& initialLayout);
};