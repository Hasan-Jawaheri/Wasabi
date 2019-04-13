#pragma once

#include "../Core/WCommon.h"
#include "WVulkanMemoryManager.h"

class WBufferedImage2D {
public:
	WBufferedImage2D();

	VkResult Create(class Wasabi* app, uint numBuffers, uint width, uint height, VkFormat format, void* pixels = nullptr, W_MEMORY_STORAGE memory = W_MEMORY_DEVICE_LOCAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);
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