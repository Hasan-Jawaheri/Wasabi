#pragma once

#include "Wasabi/Core/WCommon.hpp"
#include "Wasabi/Memory/WVulkanMemoryManager.hpp"

class WBufferedBuffer {
public:
	WBufferedBuffer();

	VkResult Create(class Wasabi* app, uint32_t numBuffers, size_t size, VkBufferUsageFlags usage, void* data = nullptr, W_MEMORY_STORAGE memory = W_MEMORY_DEVICE_LOCAL);
	void Destroy(class Wasabi* app);

	VkResult Map(class Wasabi* app, uint32_t bufferIndex, void** data, W_MAP_FLAGS flags);
	void Unmap(class Wasabi* app, uint32_t bufferIndex);

	VkBuffer GetBuffer(class Wasabi* app, uint32_t bufferIndex);

	bool Valid() const;
	size_t GetMemorySize() const;

private:
	size_t m_bufferSize;
	W_MAP_FLAGS m_lastMapFlags;

	void* m_readOnlyMemory;
	std::vector<WVulkanBuffer> m_buffers;
};