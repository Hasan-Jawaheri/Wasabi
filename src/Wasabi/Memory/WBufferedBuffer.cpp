#include "Wasabi/Memory/WBufferedBuffer.hpp"
#include "Wasabi/Core/WCore.hpp"

WBufferedBuffer::WBufferedBuffer() {
	m_lastMapFlags = W_MAP_UNDEFINED;
	m_readOnlyMemory = nullptr;
}

VkResult WBufferedBuffer::Create(Wasabi* app, uint32_t numBuffers, size_t size, VkBufferUsageFlags usage, void* data, W_MEMORY_STORAGE memory) {
	VkDevice device = app->GetVulkanDevice();
	Destroy(app);

	VkResult result = VK_SUCCESS;

	if (memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY && !data)
		return VK_ERROR_INITIALIZATION_FAILED;

	m_bufferSize = size;

	WVulkanBuffer stagingBuffer;
	for (uint32_t i = 0; i < numBuffers; i++) {
		VkMemoryPropertyFlags bufferMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // device local means only GPU can access it, more efficient
		if (data && (memory == W_MEMORY_DEVICE_LOCAL || memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY)) {
			// the buffer usage is now also a transfer destination
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		} else if (memory == W_MEMORY_HOST_VISIBLE) {
			// don't use a staging buffer for dynamic buffers, instead, just make them
			// host-visible so the CPU can directly access them
			bufferMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		}

		//
		// Create the buffer as a destination of a copy, from the staging buffer to this buffer
		// unless no staging buffer was created, then we don't need to perform any transfer.
		//
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;

		WVulkanBuffer buffer;
		result = buffer.Create(app, bufferCreateInfo, bufferMemoryFlags);
		if (result != VK_SUCCESS)
			break;
		m_buffers.push_back(buffer);

		//
		// Create a host-visible staging buffer that contains the raw buffer data.
		// we will later copy that buffer's contents into the newly created buffer.
		// If the resource is host-visible or no initial data is provided, we won't
		// need to perform the staging operation.
		//
		if (data && (memory == W_MEMORY_DEVICE_LOCAL || memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY)) {
			VkBufferCreateInfo stagingBufferCreateInfo = {};
			stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			stagingBufferCreateInfo.size = size;
			stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // This buffer is used as a transfer source for the buffer copy
			stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			result = stagingBuffer.Create(app, stagingBufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			if (result != VK_SUCCESS)
				break;

			void* pStagingMem;
			result = vkMapMemory(device, stagingBuffer.mem, 0, size, 0, &pStagingMem);
			if (result != VK_SUCCESS)
				break;
			memcpy(pStagingMem, data, size);
			vkUnmapMemory(device, stagingBuffer.mem);
		}

		//
		// Now copy the contents of the staging buffer into the buffer memory
		// and then delete the staging buffer.
		//
		if (stagingBuffer.buf) {
			result = app->MemoryManager->BeginCopyCommandBuffer();
			if (result != VK_SUCCESS)
				break;

			VkCommandBuffer copyCmdBuffer = app->MemoryManager->GetCopyCommandBuffer();

			VkBufferCopy copyRegion = {};
			copyRegion.size = size;
			vkCmdCopyBuffer(
				copyCmdBuffer,
				stagingBuffer.buf,
				buffer.buf,
				1,
				&copyRegion);

			VkBufferMemoryBarrier bufferMemoryBarrier = {};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
			bufferMemoryBarrier.buffer = buffer.buf;
			bufferMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			bufferMemoryBarrier.offset = 0;
			bufferMemoryBarrier.size = size;

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				copyCmdBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				0,
				0, nullptr,
				1, &bufferMemoryBarrier,
				0, nullptr);

			result = app->MemoryManager->EndCopyCommandBuffer(true);
			if (result != VK_SUCCESS)
				break;

			stagingBuffer.Destroy(app);

			if (memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY) {
				m_readOnlyMemory = W_SAFE_ALLOC(size);
				memcpy(m_readOnlyMemory, data, size);
			}
		} else if (data) {
			// this is a dynamic buffer with initialization info, map/unmap to initialize
			void* pMemData;
			result = Map(app, i, &pMemData, W_MAP_WRITE);
			if (result != VK_SUCCESS)
				break;
			memcpy(pMemData, data, size);
			Unmap(app, i);
		}
	}

	if (result != VK_SUCCESS) {
		stagingBuffer.Destroy(app);
		Destroy(app);
	}

	return result;
}

void WBufferedBuffer::Destroy(Wasabi* app) {
	for (auto it = m_buffers.begin(); it != m_buffers.end(); it++)
		it->Destroy(app);
	m_buffers.clear();
	m_bufferSize = 0;
	W_SAFE_FREE(m_readOnlyMemory);
}

VkResult WBufferedBuffer::Map(Wasabi* app, uint32_t bufferIndex, void** data, W_MAP_FLAGS flags) {
	VkResult result = VK_RESULT_MAX_ENUM;
	if (m_lastMapFlags == W_MAP_UNDEFINED && flags != W_MAP_UNDEFINED) {
		if (m_readOnlyMemory) {
			if (flags != W_MAP_READ)
				return result;
			*data = m_readOnlyMemory;
			return VK_SUCCESS;
		}

		VkDevice device = app->GetVulkanDevice();
		result = vkMapMemory(device, m_buffers[bufferIndex].mem, 0, m_bufferSize, 0, data);
		if (result == VK_SUCCESS)
			m_lastMapFlags = flags;
	}
	return result;
}

void WBufferedBuffer::Unmap(Wasabi* app, uint32_t bufferIndex) {
	if (m_lastMapFlags != W_MAP_UNDEFINED) {
		if (!m_readOnlyMemory) {
			VkDevice device = app->GetVulkanDevice();
			vkUnmapMemory(device, m_buffers[bufferIndex].mem);
		}

		m_lastMapFlags = W_MAP_UNDEFINED;
	}
}

VkBuffer WBufferedBuffer::GetBuffer(Wasabi* app, uint32_t bufferIndex) {
	UNREFERENCED_PARAMETER(app);

	bufferIndex = bufferIndex % m_buffers.size();
	return m_buffers[bufferIndex].buf;
}

bool WBufferedBuffer::Valid() const {
	return m_bufferSize > 0;
}

size_t WBufferedBuffer::GetMemorySize() const {
	return m_bufferSize;
}
