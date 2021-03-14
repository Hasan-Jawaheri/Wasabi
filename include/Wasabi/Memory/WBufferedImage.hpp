#pragma once

#include "Wasabi/Core/WCommon.hpp"
#include "Wasabi/Memory/WVulkanMemoryManager.hpp"

struct WBufferedImageProperties {
	VkFormat format;
	W_MEMORY_STORAGE memory;
	VkImageUsageFlags usage;
	VkSampleCountFlagBits sampleCount;
	VkImageType type;
	uint32_t arraySize;
	uint32_t mipLevels;

	WBufferedImageProperties() {}
	WBufferedImageProperties(
		VkFormat _format,
		W_MEMORY_STORAGE _memory = W_MEMORY_DEVICE_LOCAL,
		VkImageUsageFlags _usage = VK_IMAGE_USAGE_SAMPLED_BIT,
		uint32_t _arraySize = 1,
		uint32_t _mipLevels = 1,
		VkImageType _type = VK_IMAGE_TYPE_2D,
		VkSampleCountFlagBits _sampleCount = VK_SAMPLE_COUNT_1_BIT
	) : format(_format), memory(_memory), usage(_usage), sampleCount(_sampleCount), type(_type), arraySize(_arraySize), mipLevels(_mipLevels) {}
};

class WBufferedImage {
public:
	WBufferedImage();

	VkResult Create(class Wasabi* app, uint32_t numBuffers, uint32_t width, uint32_t height, uint32_t depth, WBufferedImageProperties properties, void* pixels = nullptr);
	void Destroy(class Wasabi* app);

	VkResult Map(class Wasabi* app, uint32_t bufferIndex, void** pixels, W_MAP_FLAGS flags);
	void Unmap(class Wasabi* app, uint32_t bufferIndex);

	VkImageView GetView(class Wasabi* app, uint32_t bufferIndex) const;
	VkImageLayout GetLayout(uint32_t bufferIndex) const;
	void TransitionLayoutTo(VkCommandBuffer cmdBuf, VkImageLayout newLayout, uint32_t bufferIndex);

	bool Valid() const;
	size_t GetMemorySize() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;
	uint32_t GetArraySize() const;
	WBufferedImageProperties GetProperties() const;

private:
	VkImageAspectFlags m_aspect;
	WBufferedImageProperties m_properties;
	size_t m_bufferSize;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_depth;
	W_MAP_FLAGS m_lastMapFlags;

	void* m_readOnlyMemory;
	std::vector<WVulkanImage> m_images;
	std::vector<WVulkanBuffer> m_stagingBuffers;
	std::vector<VkImageLayout> m_layouts;

	VkResult CopyStagingToImage(class Wasabi* app, WVulkanBuffer& buffer, WVulkanImage& image, VkImageLayout& initialLayout);
};