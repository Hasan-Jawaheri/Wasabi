#pragma once

#include "../Core/WCommon.h"
#include "WVulkanMemoryManager.h"

class WBufferedBuffer {
public:
	WBufferedBuffer();

	VkResult Create(class Wasabi* app, uint numBuffers, size_t size, VkBufferUsageFlags usage, void* data = nullptr, W_MEMORY_STORAGE memory = W_MEMORY_DEVICE_LOCAL);
	void WaitForFullCreation(class Wasabi* app);
	void Destroy(class Wasabi* app);

	VkResult Map(class Wasabi* app, uint bufferIndex, void** data, W_MAP_FLAGS flags);
	void Unmap(class Wasabi* app, uint bufferIndex);

	VkResult Flush(class Wasabi* app, uint bufferIndex, size_t offset = -1, size_t size = -1);
	VkResult Invalidate(class Wasabi* app, uint bufferIndex, size_t offset = -1, size_t size = -1);

	VkBuffer GetBuffer(class Wasabi* app, uint bufferIndex);

	bool Valid() const;
	size_t GetMemorySize() const;

private:
	size_t m_bufferSize;
	W_MAP_FLAGS m_lastMapFlags;

	void* m_readOnlyMemory;
	std::vector<WVulkanBuffer> m_buffers;
	mutable std::vector<WVulkanBuffer> m_stagingBuffers;
	mutable std::vector<VkFence> m_stagingCopyFences;

	void CheckCopyFence(class Wasabi* app, uint bufferIndex) const;
};