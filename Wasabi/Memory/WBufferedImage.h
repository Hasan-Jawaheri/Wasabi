#pragma once

#include "../Core/WCommon.h"
#include "WVulkanMemoryManager.h"

class WBufferedImage2D {
public:
	WBufferedImage2D();

	VkResult Create(class Wasabi* app, uint numBuffers, uint width, uint height, VkFormat format, void* texels = nullptr, W_MEMORY_STORAGE memory = W_MEMORY_DEVICE_LOCAL, VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT);
	void Destroy(class Wasabi* app);

	VkResult Map(class Wasabi* app, uint bufferIndex, void** texels, W_MAP_FLAGS flags);
	void Unmap(class Wasabi* app, uint bufferIndex);

	VkImageView GetView(class Wasabi* app, uint bufferIndex) const;

	bool Valid() const;
	size_t GetMemorySize() const;

private:
	VkImageAspectFlags m_aspect;
	size_t m_bufferSize;
	W_MAP_FLAGS m_lastMapFlags;

	void* m_readOnlyMemory;
	std::vector<WVulkanImage> m_images;
};