#include "WBufferedBuffer.h"
#include "../Core/WCore.h"

WBufferedBuffer::WBufferedBuffer() {
	m_lastMapFlags = W_MAP_UNDEFINED;
	m_readOnlyMemory = nullptr;
}

VkResult WBufferedBuffer::Create(Wasabi* app, uint numBuffers, size_t size, VkBufferUsageFlags usage, void* data, W_MEMORY_STORAGE memory) {
	VkDevice device = app->GetVulkanDevice();
	Destroy(app);

	VkResult result = VK_SUCCESS;

	if (memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY && !data)
		return VK_ERROR_INITIALIZATION_FAILED;

	for (uint i = 0; i < numBuffers; i++) {
		VkMemoryPropertyFlags bufferMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; // device local means only GPU can access it, more efficient
		if (data && (memory == W_MEMORY_DEVICE_LOCAL || memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY)) {
			// the buffer usage is now also a transfer destination
			usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		} else if (memory == W_MEMORY_HOST_VISIBLE) {
			// don't use a staging buffer for dynamic buffers, instead, just make them
			// host-visible so the CPU can directly access them
			bufferMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
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
		WVulkanBuffer stagingBuffer;
		if (data && (memory == W_MEMORY_DEVICE_LOCAL || memory == W_MEMORY_DEVICE_LOCAL_HOST_COPY)) {
			VkBufferCreateInfo stagingBufferCreateInfo = {};
			stagingBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			stagingBufferCreateInfo.size = size;
			stagingBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // This buffer is used as a transfer source for the buffer copy
			stagingBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			result = stagingBuffer.Create(app, stagingBufferCreateInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			if (result != VK_SUCCESS)
				break;
			m_stagingBuffers.push_back(stagingBuffer);

			void* pStagingMem;
			result = vkMapMemory(device, stagingBuffer.mem, 0, size, 0, &pStagingMem);
			if (result != VK_SUCCESS)
				break;
			memcpy(pStagingMem, data, size);
			vkUnmapMemory(device, stagingBuffer.mem);
		}

		//
		// Now copy the contents of the staging buffer into the buffer memory
		// If the buffer is dynamic, then this staging buffer will persist for
		// as long as the buffer does. Otherwise, we will need to delete it.
		// But we cannot delete it until the copy operation is complete, so
		// we will assign a fence to the submission of the copy command which
		// will be signalled when the copy is complete, then we will delete
		// the staging buffer later on in a call to GetView()
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
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0, nullptr,
				1, &bufferMemoryBarrier,
				0, nullptr);

			// create a fence that will be signalled when the copy is done so we can
			// then delete the staging buffer
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = 0; // the fence is not signalled
			VkFence copyFence = VK_NULL_HANDLE;
			VkResult fenceCreateResult = vkCreateFence(device, &fenceCreateInfo, nullptr, &copyFence);
			if (fenceCreateResult == VK_SUCCESS)
				m_stagingCopyFences.push_back(copyFence);

			result = app->MemoryManager->EndCopyCommandBuffer(copyFence);
			if (result != VK_SUCCESS)
				break;
			if (fenceCreateResult != VK_SUCCESS) {
				result = fenceCreateResult;
				break;
			}

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

	m_bufferSize = size;

	if (result != VK_SUCCESS)
		Destroy(app);
	return result;
}

void WBufferedBuffer::Destroy(Wasabi* app) {
	VkDevice device = app->GetVulkanDevice();
	for (auto it = m_stagingBuffers.begin(); it != m_stagingBuffers.end(); it++)
		it->Destroy(app);
	for (auto it = m_buffers.begin(); it != m_buffers.end(); it++)
		it->Destroy(app);
	for (auto it = m_stagingCopyFences.begin(); it != m_stagingCopyFences.end(); it++) {
		if (*it != VK_NULL_HANDLE)
			vkDestroyFence(device, *it, nullptr);
	}
	m_stagingBuffers.clear();
	m_buffers.clear();
	m_stagingCopyFences.clear();
	m_bufferSize = 0;
	W_SAFE_FREE(m_readOnlyMemory);
}

void WBufferedBuffer::WaitForFullCreation(Wasabi* app) {
	if (m_stagingCopyFences.size() > 0) {
		VkDevice device = app->GetVulkanDevice();
		std::vector<VkFence> fences;
		for (auto it = m_stagingCopyFences.begin(); it != m_stagingCopyFences.end(); it++) {
			if (*it != VK_NULL_HANDLE)
				fences.push_back(*it);
		}
		vkWaitForFences(device, fences.size(), fences.data(), VK_TRUE, -1);

		for (auto it = fences.begin(); it != fences.end(); it++)
			vkDestroyFence(device, *it, nullptr);
		m_stagingCopyFences.clear();
		m_stagingBuffers.clear();
	}
}

void WBufferedBuffer::CheckCopyFence(Wasabi* app, uint bufferIndex) const {
	//
	// Check if we can delete the staging buffers: First check if the fence
	// at the given index is signalled (and if so, destroy the fence and the
	// staging buffer), then check if all fences are signalled to clear the
	// vectors.
	//
	if (m_stagingCopyFences.size() > 0) {
		VkDevice device = app->GetVulkanDevice();
		VkFence fence = m_stagingCopyFences[bufferIndex];
		if (fence != VK_NULL_HANDLE && vkGetFenceStatus(device, fence) != VK_NOT_READY) {
			vkDestroyFence(device, fence, nullptr);
			m_stagingCopyFences[bufferIndex] = VK_NULL_HANDLE;
			m_stagingBuffers[bufferIndex].Destroy(app);

			bool bAllFencesSignalled = true;
			for (auto it = m_stagingCopyFences.begin(); it != m_stagingCopyFences.end(); it++) {
				if (*it != VK_NULL_HANDLE)
					bAllFencesSignalled = false;
			}
			if (bAllFencesSignalled) {
				m_stagingCopyFences.clear();
				m_stagingBuffers.clear();
			}
		}
	}
}

VkResult WBufferedBuffer::Flush(class Wasabi* app, uint bufferIndex, size_t offset, size_t size) {
	VkDevice device = app->GetVulkanDevice();
	VkMappedMemoryRange memRange = {};
	memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memRange.offset = offset == -1 ? 0 : offset;
	memRange.size = size == -1 ? m_bufferSize : size;
	memRange.memory = m_buffers[bufferIndex].mem;
	VkResult result = vkFlushMappedMemoryRanges(device, 1, &memRange);
	if (result != VK_SUCCESS)
		return result;

	result = app->MemoryManager->BeginCopyCommandBuffer();
	if (result != VK_SUCCESS)
		return result;

	VkCommandBuffer copyCmdBuffer = app->MemoryManager->GetCopyCommandBuffer();

	VkBufferMemoryBarrier bufferMemoryBarrier = {};
	bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
	bufferMemoryBarrier.buffer = m_buffers[bufferIndex].buf;
	bufferMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
	bufferMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	bufferMemoryBarrier.offset = memRange.offset;
	bufferMemoryBarrier.size = memRange.size;

	// Put barrier inside setup command buffer
	vkCmdPipelineBarrier(
		copyCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, nullptr,
		1, &bufferMemoryBarrier,
		0, nullptr);

	return app->MemoryManager->EndCopyCommandBuffer();
}

VkResult WBufferedBuffer::Invalidate(class Wasabi* app, uint bufferIndex, size_t offset, size_t size) {
	VkDevice device = app->GetVulkanDevice();
	VkMappedMemoryRange memRange = {};
	memRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memRange.offset = offset == -1 ? 0 : offset;
	memRange.size = size == -1 ? m_bufferSize : size;
	memRange.memory = m_buffers[bufferIndex].mem;
	return vkInvalidateMappedMemoryRanges(device, 1, &memRange);
}

VkResult WBufferedBuffer::Map(Wasabi* app, uint bufferIndex, void** data, W_MAP_FLAGS flags) {
	VkResult result = VK_RESULT_MAX_ENUM;
	if (m_lastMapFlags == W_MAP_UNDEFINED && flags != W_MAP_UNDEFINED) {
		if (m_readOnlyMemory) {
			if (flags != W_MAP_READ)
				return result;
			*data = m_readOnlyMemory;
			return VK_SUCCESS;
		}

		VkDevice device = app->GetVulkanDevice();
		if (flags & W_MAP_READ) {
			result = Invalidate(app, bufferIndex);
			if (result != VK_SUCCESS)
				return result;
		}
		result = vkMapMemory(device, m_buffers[bufferIndex].mem, 0, m_bufferSize, 0, data);
		if (result == VK_SUCCESS)
			m_lastMapFlags = flags;
	}
	return result;
}

void WBufferedBuffer::Unmap(Wasabi* app, uint bufferIndex) {
	if (m_lastMapFlags != W_MAP_UNDEFINED) {
		if (!m_readOnlyMemory) {
			VkDevice device = app->GetVulkanDevice();
			if (m_lastMapFlags & W_MAP_WRITE) {
				Flush(app, bufferIndex);
			}

			vkUnmapMemory(device, m_buffers[bufferIndex].mem);
		}

		m_lastMapFlags = W_MAP_UNDEFINED;
	}
}

VkBuffer WBufferedBuffer::GetBuffer(Wasabi* app, uint bufferIndex) {
	CheckCopyFence(app, bufferIndex);
	return m_buffers[bufferIndex % m_buffers.size()].buf;
}

bool WBufferedBuffer::Valid() const {
	return m_bufferSize > 0;
}

size_t WBufferedBuffer::GetMemorySize() const {
	return m_bufferSize;
}
